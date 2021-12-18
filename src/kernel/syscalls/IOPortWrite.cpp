#include <syscalls/IOPortWrite.hpp>
#include <portio.hpp>

size_t IOPortWrite(const shared_ptr<Process> &process,
                   size_t handle,
                   size_t offset,
                   size_t size,
                   size_t value,
                   size_t) {
    auto range = process->handleManager.lookup<IOPortRange>(handle);
    uint16_t port = static_cast<uint16_t>(offset) + range.start;

    if (port < range.start
            || port + size - 1 > range.max
            || !(size == 1 || size == 2 || size == 4)) {
        cout << "IOPortWrite: bad offset/size" << endl;
        throw BadUserSpace(process);
    }

    portio::write(static_cast<uint16_t>(port), size, value);
    return 0;
}

