#include <syscalls/DeleteHandle.hpp>
#include <system/Processor.hpp>

size_t DeleteHandle(const shared_ptr<Process> &process,
                            size_t handle,
                            size_t,
                            size_t,
                            size_t,
                            size_t) {
    shared_ptr<Thread> removedThread;
    process->handleManager.removeHandle(handle, removedThread);
    if (removedThread) {
        if (removedThread.get() == CurrentThread()) {
            cout << "removed thread was active thread. returning" << endl;
            throw ThreadKilled();
        } else {
            scoped_lock sl(KernelInterruptsLock);
            cout << "removed thread was not active thread. sending terminate" << endl;
            removedThread->terminate();
        }
    }
    return 0;
}

