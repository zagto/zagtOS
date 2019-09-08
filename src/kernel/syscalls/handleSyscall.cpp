#include <lib/Lock.hpp>
#include <lib/vector.hpp>
#include <memory/UserSpaceObject.hpp>
#include <system/System.hpp>
#include <tasks/Thread.hpp>
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
        delete this;
        return true;
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
