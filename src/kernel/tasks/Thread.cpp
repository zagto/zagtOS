#include <memory/UserSpaceObject.hpp>
#include <common/common.hpp>
#include <memory/UserSpaceObject.hpp>
#include <tasks/Thread.hpp>
#include <tasks/Task.hpp>
#include <tasks/MMAP.hpp>
#include <system/System.hpp>


Thread::~Thread() {
    Assert(currentProcessor == CurrentProcessor);
    /* don't try deleting special threads */
    Assert(task);

    CurrentProcessor->scheduler.remove(this);
    task->removeThread(this);
}

bool Thread::handleSyscall() {
    switch (registerState.syscallNr()) {
    case SYS_LOG: {
        LockHolder lh(task->pagingLock);
        static const usize MAX_LOG_SIZE = 10000;
        usize address = registerState.syscallParameter(1);
        usize length = registerState.syscallParameter(0);

        if (length > MAX_LOG_SIZE) {
            Log << "Task attempted to send huge log. ingnoring.\n";
            return true;
        }

        Vector<u8> buffer(MAX_LOG_SIZE);
        bool valid = task->copyFromUser(&buffer[0], address, length, false);
        if (!valid) {
            Log << "SYS_LOG: invalid buffer\n";
            return false;
        }
        for (usize index = 0; index < buffer.size(); index++) {
            log::Log << static_cast<char>(buffer[index]);
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
        Assert(task);
        UserSpaceObject<MMAP, USOOperation::READ_AND_WRITE> uso(registerState.syscallParameter(0),
                                                                task);
        if (!uso.valid) {
            return false;
        }
        uso.object.perform(*task);
        return true;
    }
    default:
        return false;
    }
}
