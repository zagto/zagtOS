#include <syscalls/IOPortRead.hpp>
#include <portio.hpp>

size_t IOPortRead(const shared_ptr<Process> &process,
                          size_t port,
                          size_t size,
                          size_t,
                          size_t,
                          size_t) {
    if (process->canAccessPhysicalMemory()) {
        return portio::read(static_cast<uint16_t>(port), size);
    } else {
        throw BadUserSpace(process);
    }
}
