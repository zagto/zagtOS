#include <syscalls/IOPortWrite.hpp>
#include <portio.hpp>

size_t IOPortWrite(const shared_ptr<Process> &process,
                           size_t port,
                           size_t size,
                           size_t value,
                           size_t,
                           size_t) {
    if (!process->canAccessPhysicalMemory()) {
        throw BadUserSpace(process);
    }

    portio::write(static_cast<size_t>(port), size, value);
    return 0;
}

