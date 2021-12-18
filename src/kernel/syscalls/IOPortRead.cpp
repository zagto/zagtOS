#include <syscalls/IOPortRead.hpp>
#include <portio.hpp>

size_t IOPortRead(const shared_ptr<Process> &process,
                  size_t handle,
                  size_t offset,
                  size_t size,
                  size_t,
                  size_t) {
    auto range = process->handleManager.lookup<IOPortRange>(handle);
    uint16_t port = static_cast<uint16_t>(offset) + range.start;

    if (port < range.start
            || port + size - 1 > range.max
            || !(size == 1 || size == 2 || size == 4)) {
        cout << "IOPortRead: bad offset/size" << endl;
        throw BadUserSpace(process);
    }

    return portio::read(static_cast<uint16_t>(port), size);
}
