#include <syscalls/CreateThread.hpp>
#include <processes/Scheduler.hpp>

Result<size_t> CreateThread(const shared_ptr<Process> &process,
                            size_t entry,
                            size_t stack,
                            size_t priority,
                            size_t tls,
                            size_t) {
    scoped_lock lg1(process->pagingLock);

    Thread::Priority actualPriority;
    if (priority == Thread::KEEP_PRIORITY) {
        actualPriority = CurrentThread()->ownPriority();
    } else if (priority >= Thread::NUM_PRIORITIES) {
        cout << "SYS_CREATE_THREAD: invalid priority " << priority << endl;
        return Status::BadUserSpace();
    } else {
        actualPriority = static_cast<Thread::Priority>(priority);
    }

    Result newThread = make_shared<Thread>(process, entry, actualPriority, stack, tls);
    if (!newThread) {
        return newThread.status();
    }

    Result<uint32_t> handle = process->handleManager.addThread(*newThread);
    if (!handle) {
        return handle.status();
    }
    Scheduler::schedule(newThread->get());
    cout << "created Thread with handle " << *handle << endl;
    return *handle;
}

