#include <memory/UserSpaceObject.hpp>
#include <common/common.hpp>
#include <memory/UserSpaceObject.hpp>
#include <tasks/Thread.hpp>
#include <tasks/Task.hpp>
#include <tasks/MMAP.hpp>
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
            log::cout << static_cast<char>(buffer[index]);
        }
        return true;
    }
    case SYS_EXIT:
        delete this;
        return true;
    case SYS_RANDOM:
        //registerState()->setSyscallResult(random());
        // todo: should write to memory here
        return true;

    case SYS_MMAP: {
        LockHolder lh(task->pagingLock);
        UserSpaceObject<MMAP, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                task);
        if (!uso.valid) {
            return false;
        }
        uso.object.perform(*task);
        return true;
    }
    case SYS_GET_ACPI_ROOT: {
        /* TODO: permissions checking */
        cout << "returning acpi root: " << CurrentSystem.ACPIRoot.value() << endl;
        registerState.setSyscallResult(CurrentSystem.ACPIRoot.value());
        return true;
    }
    default:
        return false;
    }
}
