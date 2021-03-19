#include <syscalls/IOPortWrite.hpp>
#include <portio.hpp>

Result<size_t> IOPortWrite(const shared_ptr<Process> &process,
                           size_t port,
                           size_t size,
                           size_t value,
                           size_t,
                           size_t) {
    if (!process->canAccessPhysicalMemory()) {
        return Status::BadUserSpace();
    }

    Status status = portio::write(static_cast<size_t>(port), size, value);
    if (status) {
        return 0;
    } else {
        return status;
    }
}

