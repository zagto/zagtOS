#include <lib/Lock.hpp>
#include <lib/vector.hpp>
#include <memory/UserSpaceObject.hpp>
#include <system/System.hpp>
#include <tasks/Thread.hpp>
#include <tasks/Port.hpp>
#include <syscalls/MappingOperation.hpp>
#include <syscalls/SpawnProcess.hpp>
#include <syscalls/SyscallNumbers.hpp>


bool Thread::handleSyscall() {
    switch (registerState.syscallNr()) {
    case SYS_LOG: {
        LockHolder lh(task->pagingLock);
        static const size_t MAX_LOG_SIZE = 10000;
        size_t address = registerState.syscallParameter(1);
        size_t length = registerState.syscallParameter(0);

        if (length > MAX_LOG_SIZE) {
            cout << "Task attempted to send huge log. ingnoring.\n";
            return true;
        }

        if (length == 0) {
            return true;
        }

        vector<uint8_t> buffer(length);
        bool valid = task->copyFromUser(&buffer[0], address, length, false);
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
        LockHolder lh(task->pagingLock);
        LockHolder lh2(task->portsLock);
        size_t tagsAddress = registerState.syscallParameter(0);
        size_t numTags = registerState.syscallParameter(1);

        vector<uint32_t> acceptedTags(numTags);
        if (numTags > 0) {
            bool valid = task->copyFromUser(reinterpret_cast<uint8_t *>(acceptedTags.data()),
                                            tagsAddress,
                                            numTags * sizeof(uint32_t),
                                            false);
            if (!valid) {
                return false;
            }
        }

        task->ports.push_back(new Port(acceptedTags));
        registerState.setSyscallResult(task->ports[task->ports.size() - 1]->id());
        cout << "created port " << task->ports[task->ports.size() - 1]->id() << endl;
        return true;
    }
    case SYS_DESTROY_PORT: {
        LockHolder lh(task->portsLock);
        uint32_t id = static_cast<uint32_t>(registerState.syscallParameter(0));

        cout << "deleting port " << id << endl;

        for (size_t i = 0; i < task->ports.size(); i++) {
            Port *port = task->ports[i];
            if (port->id() == id) {
                task->ports.remove(port);
                cout << "success" << endl;
                delete port;
                return true;
            }
        }
        cout << "not found" << endl;
        return false;
    }
    case SYS_RANDOM:
        // todo: should write to memory here
        // 0 = pointer,
        // 1 = length
        return true;

    case SYS_MMAP: {
        LockHolder lh(task->pagingLock);
        UserSpaceObject<MMap, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                task);
        if (!uso.valid) {
            return false;
        }
        uso.object.perform(*task);
        return true;
    }
    case SYS_MUNMAP: {
        LockHolder lh(task->pagingLock);
        MUnmap munmap(registerState.syscallParameter(0), registerState.syscallParameter(1));
        munmap.perform(*task);
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
        LockHolder lh(task->pagingLock);
        UserSpaceObject<SpawnProcess, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                    task);
        if (!uso.valid) {
            return false;
        }
        return uso.object.perform(*task);
    }
    default:
        return false;
    }
}
