#include <memory/UserSpaceObject.hpp>
#include <common/common.hpp>
#include <memory/UserSpaceObject.hpp>
#include <tasks/Thread.hpp>
#include <tasks/Task.hpp>
#include <tasks/MappingOperation.hpp>
#include <system/System.hpp>


Thread::~Thread() {
    assert(currentProcessor == CurrentProcessor);
    /* don't try deleting special threads */
    assert(task);

    CurrentProcessor->scheduler.remove(this);
    task->removeThread(this);
}


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
        /* TODO: permissions checking */
        LockHolder lh(task->pagingLock);
        size_t address = registerState.syscallParameter(0);
        size_t length = registerState.syscallParameter(1);
        size_t priority = registerState.syscallParameter(2);
        size_t messageAddress = registerState.syscallParameter(3);

        /* make all the returns in the error handlers return 0 */
        registerState.setSyscallResult(0);

        if (priority >= NUM_PRIORITIES) {
            cout << "SYS_SPAWN_PROCESS: invalid priority\n";
            return true;
        }

        vector<uint8_t> buffer(length);
        bool valid = task->copyFromUser(&buffer[0], address, length, false);
        if (!valid) {
            cout << "SYS_SPAWN_PROCESS: invalid buffer\n";
            return true;
        }

        ELF elf{Slice<vector, uint8_t>(&buffer)};
        if (!valid) {
            return true;
        }

        UserSpaceObject<Object, USOOperation::READ> messageHeader(messageAddress, task);
        if (!messageHeader.valid) {
            cout << "SYS_SPAWN_PROCESS: could not access message header\n";
            return true;
        }

        size_t messageLength = messageHeader.object.sizeInMemory();
        valid = task->verifyUserAccess(messageAddress, messageLength, false);
        if (!valid) {
            cout << "SYS_SPAWN_PROCESS: invalid message\n";
            return true;
        }

        Task *newTask = new Task(elf, static_cast<Thread::Priority>(priority), messageLength);
        LockHolder lh2(newTask->pagingLock);
        valid = newTask->copyFromOhterUserSpace(newTask->runMessageAddress.value(),
                                               task,
                                               messageAddress,
                                               messageLength);
        /* the checks before should have caught everything */
        assert(valid);

        registerState.setSyscallResult(1);
        return true;
    }
    default:
        return false;
    }
}
