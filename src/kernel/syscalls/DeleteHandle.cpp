#include <syscalls/DeleteHandle.hpp>

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
        removedThread->terminate();
    }
    return 0;
}

