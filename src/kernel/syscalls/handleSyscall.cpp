#include <mutex>
#include <vector>
#include <memory/UserSpaceObject.hpp>
#include <system/System.hpp>
#include <processes/Thread.hpp>
#include <processes/Port.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <syscalls/Time.hpp>
#include <syscalls/Futex.hpp>
#include <syscalls/SyscallNumbers.hpp>
#include <portio.hpp>
#include <system/Processor.hpp>


bool Thread::handleSyscall() {
    switch (registerState.syscallNumber()) {
    case SYS_LOG: {
        scoped_lock lg(process->pagingLock);
        static const size_t MAX_LOG_SIZE = 10000;
        size_t address = registerState.syscallParameter(0);
        size_t length = registerState.syscallParameter(1);

        if (length > MAX_LOG_SIZE) {
            cout << "Process attempted to send huge log. ignoring.\n";
            return true;
        }

        if (length == 0) {
            return true;
        }

        vector<uint8_t> buffer(length);
        bool valid = process->copyFromUser(&buffer[0], address, length, false);
        if (!valid) {
            cout << "SYS_LOG: invalid buffer\n";
            return false;
        }
        cout.setProgramColor();
        for (size_t index = 0; index < buffer.size(); index++) {
            cout << static_cast<char>(buffer[index]);
        }
        cout.setKernelColor();
        return true;
    }
    case SYS_EXIT:
        cout << "Process Exit" << endl;
        /* Danger - the current thread (this) will be deleted */
        process->exit();
        return true;
    case SYS_CREATE_PORT: {
        shared_ptr<Port> port = make_shared<Port>(process);
        uint32_t handle = process->handleManager.addPort(port);
        registerState.setSyscallResult(handle);

        cout << "created port " << handle << endl;
        return true;
    }
    case SYS_SEND_MESSAGE: {
        cout << "sendMessage" << endl;
        uint32_t handle = static_cast<uint32_t>(registerState.syscallParameter(0));
        size_t messageTypeAddress = registerState.syscallParameter(1);
        size_t messageAddress = registerState.syscallParameter(2);
        size_t messageSize = registerState.syscallParameter(3);
        size_t numMessageHandles = registerState.syscallParameter(4);

        optional<weak_ptr<Port>> weakPort = process->handleManager.lookupRemotePort(handle);
        if (!weakPort) {
            cout << "sendMessage: invalid port handle " << handle << endl;
            return false;
        }

        registerState.setSyscallResult(0);
        shared_ptr<Port> port = weakPort->lock();
        if (!port) {
            cout << "sendMessage: destination port no longer exists: " << handle << endl;
            return false;
        }

        scoped_lock sl(process->pagingLock, port->process->pagingLock);
        if (!process->verifyMessageAccess(messageAddress, messageSize, numMessageHandles)) {
            return false;
        }
        UserSpaceObject<UUID, USOOperation::READ> messageType(messageTypeAddress, process);
        if (!messageType.valid) {
            return false;
        }

        unique_ptr<Message> message = make_unique<Message>(process.get(),
                                                           port->process.get(),
                                                           messageAddress,
                                                           messageType.object,
                                                           messageSize,
                                                           numMessageHandles);
        message->transfer();
        port->addMessage(move(message));
        return true;
    }
    case SYS_RECEIVE_MESSAGE: {
        cout << "receiveMessage" << endl;
        uint32_t portHandle = static_cast<uint32_t>(registerState.syscallParameter(0));
        optional<shared_ptr<Port>> port = process->handleManager.lookupPort(portHandle);
        if (!port) {
            cout << "receiveMessage: invalid port handle " << portHandle << endl;
            return false;
        }

        unique_ptr<Message> msg = (*port)->getMessageOrMakeThreadWait(this);
        if (msg) {
            registerState.setSyscallResult(msg->infoAddress().value());
        }
        return true;
    }
    case SYS_DELETE_HANDLE: {
        uint32_t handle = static_cast<uint32_t>(registerState.syscallParameter(0));
        cout << "SYS_DELETE_HANDLE " << handle << endl;
        shared_ptr<Thread> removedThread;
        bool success = process->handleManager.removeHandle(handle, removedThread);
        if (!success) {
            return false;
        }
        if (removedThread) {
            removedThread->terminate();
        }
        return true;
    }
    case SYS_RANDOM:
        // todo: should write to memory here
        // 0 = pointer,
        // 1 = length
        return true;

    case SYS_MPROTECT: {
        scoped_lock lg(process->pagingLock);
        MProtect mprotect(registerState.syscallParameter(0),
                          registerState.syscallParameter(1),
                          registerState.syscallParameter(2));
        mprotect.perform(*process);
        registerState.setSyscallResult(mprotect.error);
        return true;
    }
    case SYS_MMAP: {
        scoped_lock lg(process->pagingLock);
        UserSpaceObject<MMap, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                process);
        if (!uso.valid) {
            return false;
        }
        uso.object.perform(*process);
        return true;
    }
    case SYS_MUNMAP: {
        scoped_lock lg(process->pagingLock);
        MUnmap munmap(registerState.syscallParameter(0), registerState.syscallParameter(1));
        munmap.perform(*process);
        registerState.setSyscallResult(munmap.error);
        return true;
    }
    case SYS_CREATE_THREAD: {
        scoped_lock lg1(process->pagingLock);
        scoped_lock lg2(stateLock);
        size_t entry = registerState.syscallParameter(0);
        size_t stack = registerState.syscallParameter(1);
        uint32_t priority = registerState.syscallParameter(2);
        size_t tls = registerState.syscallParameter(3);

        Thread::Priority actualPriority;
        if (priority == Thread::KEEP_PRIORITY) {
            actualPriority = _ownPriority;
        } else if (priority >= Thread::NUM_PRIORITIES) {
            cout << "SYS_CREATE_THREAD: invalid priority " << priority << endl;
            return true;
        } else {
            actualPriority = static_cast<Thread::Priority>(priority);
        }

        auto newThread = make_shared<Thread>(process, entry, actualPriority, stack, tls);
        uint32_t handle = process->handleManager.addThread(newThread);
        Scheduler::schedule(newThread.get());
        registerState.setSyscallResult(handle);
        cout << "Created Thread " << handle << endl;
        return true;
    }
    case SYS_FUTEX: {
        return Futex(this, registerState);
    }
    case SYS_CLOCK_GETTIME:
        return GetTime(registerState, process);
    case SYS_GET_ACPI_ROOT: {
        /* TODO: permissions checking */
        registerState.setSyscallResult(CurrentSystem.ACPIRoot.value());
        return true;
    }
    case SYS_IO_PORT_READ: {
        /* TODO: permissions checking */
        uint32_t result = portio::read(static_cast<uint16_t>(registerState.syscallParameter(0)),
                                       registerState.syscallParameter(1));
        registerState.setSyscallResult(result);
        return true;
    }
    case SYS_IO_PORT_WRITE: {
        /* TODO: permissions checking */
        portio::write(static_cast<uint16_t>(registerState.syscallParameter(0)),
                      registerState.syscallParameter(1),
                      static_cast<uint32_t>(registerState.syscallParameter(2)));
        return true;
    }
    case SYS_ADD_PROCESSOR: {
        /* TODO: permissions checking */
        size_t hardwareID = registerState.syscallParameter(0);
        CurrentProcessor->interrupts.wakeSecondaryProcessor(hardwareID);
        return true;
    }
    case SYS_SPAWN_PROCESS: {
        scoped_lock lg(process->pagingLock);
        UserSpaceObject<SpawnProcess, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                    process);
        if (!uso.valid) {
            return false;
        }
        return uso.object.perform(process);
    }
    default:
        return false;
    }
}
