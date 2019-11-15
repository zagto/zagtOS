#include <lib/Lock.hpp>
#include <lib/vector.hpp>
#include <memory/UserSpaceObject.hpp>
#include <system/System.hpp>
#include <processes/Thread.hpp>
#include <processes/Port.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <syscalls/SyscallNumbers.hpp>


bool Thread::handleSyscall() {
    switch (registerState.syscallNr()) {
    case SYS_LOG: {
        LockHolder lh(process->pagingLock);
        static const size_t MAX_LOG_SIZE = 10000;
        size_t address = registerState.syscallParameter(1);
        size_t length = registerState.syscallParameter(0);

        if (length > MAX_LOG_SIZE) {
            cout << "Process attempted to send huge log. ingnoring.\n";
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
        for (size_t index = 0; index < buffer.size(); index++) {
            cout << static_cast<char>(buffer[index]);
        }
        return true;
    }
    case SYS_EXIT:
        cout << "Thread Exit" << endl;
        delete this;
        return true;
    case SYS_CREATE_PORT: {
        LockHolder lh(process->pagingLock);
        LockHolder lh2(process->portsLock);
        size_t tagsAddress = registerState.syscallParameter(0);
        size_t numTags = registerState.syscallParameter(1);

        vector<uint32_t> acceptedTags(numTags);
        if (numTags > 0) {
            bool valid = process->copyFromUser(reinterpret_cast<uint8_t *>(acceptedTags.data()),
                                            tagsAddress,
                                            numTags * sizeof(uint32_t),
                                            false);
            if (!valid) {
                return false;
            }
        }

        process->ports.push_back(new Port(*process, acceptedTags));
        registerState.setSyscallResult(process->ports[process->ports.size() - 1]->id());
        cout << "created port " << process->ports[process->ports.size() - 1]->id() << endl;
        return true;
    }
    case SYS_RECEIVE_MESSAGE: {
        LockHolder lh(process->portsLock);
        Port *port(process->getPortById(static_cast<uint32_t>(registerState.syscallParameter(0))));

        if (port == nullptr) {
            cout << "SYS_RECEIVE_MESSAGE: invalid port ID" << endl;
            return false;
        } else {
            /*if (port->messagesPending()) {
                TODO
            }*/
            return true;
        }
    }
    case SYS_DESTROY_PORT: {
        LockHolder lh(process->portsLock);
        Port *port(process->getPortById(static_cast<uint32_t>(registerState.syscallParameter(0))));

        if (port == nullptr) {
            cout << "SYS_DESTROY_PORT: invalid port ID" << endl;
            return false;
        } else {
            delete port;
            return true;
        }
    }
    case SYS_RANDOM:
        // todo: should write to memory here
        // 0 = pointer,
        // 1 = length
        return true;

    case SYS_MMAP: {
        LockHolder lh(process->pagingLock);
        UserSpaceObject<MMap, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                process);
        if (!uso.valid) {
            return false;
        }
        uso.object.perform(*process);
        return true;
    }
    case SYS_MUNMAP: {
        LockHolder lh(process->pagingLock);
        MUnmap munmap(registerState.syscallParameter(0), registerState.syscallParameter(1));
        munmap.perform(*process);
        registerState.setSyscallResult(munmap.error);
        return true;
    }
    case SYS_GET_ACPI_ROOT: {
        /* TODO: permissions checking */
        registerState.setSyscallResult(CurrentSystem.ACPIRoot.value());
        return true;
    }
    case SYS_ADD_PROCESSOR: {
        /* TODO: permissions checking */
        size_t hardwareID = registerState.syscallParameter(0);
        CurrentProcessor->interrupts.wakeSecondaryProcessor(hardwareID);
        return true;
    }
    case SYS_SPAWN_PROCESS: {
        LockHolder lh(process->pagingLock);
        UserSpaceObject<SpawnProcess, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                    process);
        if (!uso.valid) {
            return false;
        }
        return uso.object.perform(*process);
    }
    default:
        return false;
    }
}
