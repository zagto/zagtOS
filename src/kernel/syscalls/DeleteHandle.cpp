#include <syscalls/DeleteHandle.hpp>
#include <system/Processor.hpp>

Result<size_t> DeleteHandle(const shared_ptr<Process> &process,
                            size_t handle,
                            size_t,
                            size_t,
                            size_t,
                            size_t) {
    shared_ptr<Thread> removedThread;
    Status status = process->handleManager.removeHandle(handle, removedThread);
    if (!status) {
        return status;
    }
    if (removedThread) {
        if (removedThread.get() == CurrentThread()) {
            cout << "removed thread was active thread. returning" << endl;

            return Status::ThreadKilled();
        } else {
            scoped_lock sl(KernelInterruptsLock);
            cout << "removed thread was not active thread. sending terminate" << endl;
            removedThread->terminate();
        }
    }
    return 0;
}

