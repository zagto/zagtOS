#include <syscalls/CreateThread.hpp>
#include <processes/Scheduler.hpp>

size_t CreateThread(const shared_ptr<Process> &process,
                            size_t entry,
                            size_t stack,
                            size_t priority,
                            size_t tls,
                            size_t) {
    Thread::Priority actualPriority;
    if (priority == Thread::KEEP_PRIORITY) {
        actualPriority = CurrentThread()->ownPriority();
    } else if (priority >= Thread::NUM_PRIORITIES) {
        cout << "SYS_CREATE_THREAD: invalid priority " << priority << endl;
        throw BadUserSpace(process);
    } else {
        actualPriority = static_cast<Thread::Priority>(priority);
    }

    auto newThread = make_shared<Thread>(process, entry, actualPriority, stack, tls);
    uint32_t handle = process->handleManager.addThread(newThread);

    Scheduler::schedule(newThread.get(), true);
    cout << "created Thread with handle " << handle << endl;
    return handle;
}

