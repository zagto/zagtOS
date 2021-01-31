#include <mutex>
#include <vector>
#include <memory/UserSpaceObject.hpp>
#include <system/System.hpp>
#include <processes/Thread.hpp>
#include <processes/Port.hpp>
#include <processes/MemoryArea.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <syscalls/Time.hpp>
#include <syscalls/Futex.hpp>
#include <syscalls/SyscallNumbers.hpp>
#include <portio.hpp>
#include <system/Processor.hpp>
#include <interrupts/Interrupts.hpp>


extern "C" __attribute__((noreturn))
bool Syscall(uint64_t syscallNr,
             uint64_t arg0,
             int64_t arg1,
             uint64_t arg2,
             uint64_t arg3,
             uint64_t arg4,
             uint64_t) {
    Thread *thread = CurrentProcessor->scheduler.activeThread();
    shared_ptr<Process> process = thread->process;

    size_t result = 0;

    switch (syscallNr) {
    case SYS_LOG: {
        scoped_lock lg(process->pagingLock);
        static const size_t MAX_LOG_SIZE = 10000;
        size_t address = arg0;
        size_t length = arg1;

        if (length > MAX_LOG_SIZE) {
            cout << "Process attempted to send huge log. ignoring.\n";
            break;
        }

        if (length == 0) {
            break;
        }

        vector<uint8_t> buffer(length);
        bool valid = process->copyFromUser(&buffer[0], address, length, false);
        if (!valid) {
            cout << "SYS_LOG: invalid buffer\n";
            Panic(); // TODO: exception
        }
        /* do not print program name for small invisible stuff */
        if (!(length == 0 || (length == 1 && buffer[0] <= ' '))) {
            cout.setProgramNameColor();
            for (uint8_t character: process->logName) {
                cout << static_cast<char>(character);
            }
            cout.setProgramColor();
            cout << ": ";
        } else {
            cout.setProgramColor();
        }
        for (uint8_t character: buffer) {
            cout << static_cast<char>(character);
        }
        cout.setKernelColor();
        break;
    }
    case SYS_EXIT:
        cout << "Process Exit" << endl;
        /* Danger - the current thread (this) will be deleted */
        process->exit();
        break;
    case SYS_CRASH:
        /* Danger - the current thread (this) will be deleted */
        process->crash("self-termination by syscall", thread);
        break;
    case SYS_CREATE_PORT: {
        shared_ptr<Port> port = make_shared<Port>(process);
        uint32_t handle = process->handleManager.addPort(port);
        result = handle;
        break;
    }
    case SYS_SEND_MESSAGE: {
        uint32_t handle = static_cast<uint32_t>(arg0);
        size_t messageTypeAddress = arg1;
        size_t messageAddress = arg2;
        size_t messageSize = arg3;
        size_t numMessageHandles = arg4;

        optional<weak_ptr<Port>> weakPort = process->handleManager.lookupRemotePort(handle);
        if (!weakPort) {
            cout << "sendMessage: invalid port handle " << handle << endl;
            Panic(); // TODO: exception
        }

        shared_ptr<Port> port = weakPort->lock();
        if (!port) {
            cout << "sendMessage: destination port no longer exists: " << handle << endl;
            Panic(); // TODO: exception
        }

        scoped_lock sl(process->pagingLock, port->process->pagingLock);
        if (!process->verifyMessageAccess(messageAddress, messageSize, numMessageHandles)) {
            Panic(); // TODO: exception
        }
        UserSpaceObject<UUID, USOOperation::READ> messageType(messageTypeAddress, process);
        if (!messageType.valid) {
            Panic(); // TODO: exception
        }

        unique_ptr<Message> message = make_unique<Message>(process.get(),
                                                           port->process.get(),
                                                           messageAddress,
                                                           messageType.object,
                                                           messageSize,
                                                           numMessageHandles);
        message->transfer();
        port->addMessage(move(message));
        break;
    }
    case SYS_RECEIVE_MESSAGE: {
        uint32_t portHandle = static_cast<uint32_t>(arg0);
        optional<shared_ptr<Port>> port = process->handleManager.lookupPort(portHandle);
        if (!port) {
            cout << "receiveMessage: invalid port handle " << portHandle << endl;
            Panic(); // TODO: exception
        }

        unique_ptr<Message> msg = (*port)->getMessageOrMakeThreadWait(thread);
        if (msg) {
            result = msg->infoAddress.value();
        }
        break;
    }
    case SYS_DELETE_HANDLE: {
        uint32_t handle = static_cast<uint32_t>(arg0);
        shared_ptr<Thread> removedThread;
        bool success = process->handleManager.removeHandle(handle, removedThread);
        if (!success) {
            Panic(); // TODO: exception
        }
        if (removedThread) {
            removedThread->terminate();
        }
        break;
    }
    case SYS_RANDOM:
        // todo: should write to memory here
        // 0 = pointer,
        // 1 = length
        break;

    case SYS_MPROTECT: {
        scoped_lock lg(process->pagingLock);
        MProtect mprotect(arg0, arg1, arg2);
        mprotect.perform(*process);
        result = mprotect.error;
        break;
    }
    case SYS_MMAP: {
        scoped_lock lg(process->pagingLock);
        UserSpaceObject<MMap, USOOperation::READ_AND_WRITE> uso(arg0, process);
        if (!uso.valid) {
            cout << "SYS_MMAP: process passed non-accessible regions as parameters structure" << endl;
            Panic(); // TODO: exception
        }
        uso.object.perform(*process);
        break;
    }
    case SYS_MUNMAP: {
        scoped_lock lg(process->pagingLock);
        MUnmap munmap(arg1, arg2, arg0);
        munmap.perform(*process);
        result = munmap.error;
        break;
    }
    case SYS_CREATE_SHARED_MEMORY: {
        size_t type = arg0;
        size_t offset = arg1;
        size_t length = align(arg2, PAGE_SIZE, AlignDirection::UP);
        size_t deviceAddressesPointer = arg3;

        /* TODO: maybe don't hardcode permissions */
        shared_ptr<MemoryArea> memoryArea;
        switch (type) {
        case 0:
            /* Standard */
            if (offset != 0) {
                cout << "SYS_CREATE_SHARED_MEMORY: offset given for standard shared memory" << endl;
                Panic(); // TODO: exception
            }
            memoryArea = make_shared<MemoryArea>(Permissions::READ_WRITE, length);
            break;
        case 1:
            /* Physical */
            if (!process->canAccessPhysicalMemory()) {
                cout << "SYS_CREATE_SHARED_MEMORY: process which is not allowed to use physical "
                    << "memory tried to use it" << endl;
                Panic(); // TODO: exception
            }
            if (offset % PAGE_SIZE != 0) {
                cout << "SYS_CREATE_SHARED_MEMORY: offset not aligned" << endl;
                Panic(); // TODO: exception
            }
            memoryArea = make_shared<MemoryArea>(Permissions::READ_WRITE, offset, length);
            break;
        case 2: {
            /* DMA */
            scoped_lock sl(process->pagingLock);
            size_t numPages = length / PAGE_SIZE;

            int frameStack = hos_v1::DMAZone::COUNT - 1;
            while (offset > hos_v1::DMAZoneMax[frameStack]) {
                frameStack--;
                if (frameStack < 0) {
                    cout << "SYS_CREATE_SHARED_MEMORY: requested device address ceiling impossible"
                         << "on this architecture." << endl;
                    Panic(); // TODO: exception
                }
            }

            vector<size_t> deviceAddresses;
            memoryArea = make_shared<MemoryArea>(frameStack, length, deviceAddresses);
            if (!process->copyToUser(deviceAddressesPointer,
                                     reinterpret_cast<uint8_t *>(deviceAddresses.data()),
                                     numPages * sizeof(size_t),
                                     true)) {
                Panic(); // TODO: exception
            }
            break;
        }
        default:
            cout << "SYS_CREATE_SHARED_MEMORY: got invalid type parameter " << type << endl;
            Panic(); // TODO: exception
        }
        uint32_t handle = process->handleManager.addMemoryArea(memoryArea);
        result = handle;
        break;
    }
    case SYS_CREATE_THREAD: {
        scoped_lock lg1(process->pagingLock);
        size_t entry = arg0;
        size_t stack = arg1;
        uint32_t priority = arg2;
        size_t tls = arg3;

        Thread::Priority actualPriority;
        if (priority == Thread::KEEP_PRIORITY) {
            actualPriority = thread->ownPriority();
        } else if (priority >= Thread::NUM_PRIORITIES) {
            cout << "SYS_CREATE_THREAD: invalid priority " << priority << endl;
            Panic(); // TODO: exception
        } else {
            actualPriority = static_cast<Thread::Priority>(priority);
        }

        auto newThread = make_shared<Thread>(process, entry, actualPriority, stack, tls);
        uint32_t handle = process->handleManager.addThread(newThread);
        Scheduler::schedule(newThread.get());
        result = handle;
        cout << "Created Thread " << handle << endl;
        break;
    }
    case SYS_FUTEX:
        Futex(thread, arg0, arg1, arg2, arg3);
        break;
    case SYS_CLOCK_GETTIME:
        GetTime(process, arg0, arg1);
        break;
    case SYS_GET_ACPI_ROOT: {
        /* TODO: permissions checking */
        result = CurrentSystem.ACPIRoot.value();
        break;
    }
    case SYS_IO_PORT_READ: {
        /* TODO: permissions checking */
        result = portio::read(static_cast<uint16_t>(arg0), arg1);
        break;
    }
    case SYS_IO_PORT_WRITE: {
        /* TODO: permissions checking */
        portio::write(static_cast<uint16_t>(arg0),
                      arg1,
                      static_cast<uint32_t>(arg2));
        break;
    }
    case SYS_ADD_PROCESSOR: {
        /* TODO: permissions checking */
        size_t hardwareID = arg0;
        CurrentProcessor->interrupts.wakeSecondaryProcessor(hardwareID);
        break;
    }
    case SYS_SPAWN_PROCESS: {
        scoped_lock lg(process->pagingLock);
        UserSpaceObject<SpawnProcess, USOOperation::READ_AND_WRITE> uso(arg0,
                                                                    process);
        if (!uso.valid) {
            Panic(); // TODO: exception
        }
        if (uso.object.perform(process)) {
            break;
        } else {
            Panic(); // TODO: exception
        }
    }
    default:
        Panic(); // TODO: exception
    };
    thread->registerState.setSyscallResult(result);
    CurrentProcessor->interrupts.returnToUserMode();
}
