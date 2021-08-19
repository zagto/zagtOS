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
        scoped_lock sl(Processor::kernelInterruptsLock);
        if (removedThread.get() == CurrentProcessor->scheduler.activeThread()) {
            cout << "removed thread was active thread. returning" << endl;
            return Status::ThreadKilled();
        } else {
            cout << "removed thread was not active thread. sending terminate" << endl;
            removedThread->terminate();
        }
    }
    return 0;
}

