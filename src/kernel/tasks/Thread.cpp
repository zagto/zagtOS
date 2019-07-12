#include <memory/UserSpaceWindow.h>
#include <common/common.hpp>
#include <memory/UserSpaceWindow.h>
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
        UserSpaceWindow window(registerState.syscallParameter(1),
                               registerState.syscallParameter(0));
        if (!window.isValid()) {
            return false;
        }
        for (usize index = 0; index < window.size(); index++) {
            log::Log << static_cast<char>(window[index]);
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
        UserSpaceObject<MMAP> uso(registerState.syscallParameter(0));
        if (!uso.isValid()) {
            return false;
        }
        uso.object()->perform(*task);
        return true;
    }
    default:
        return false;
    }
}
