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

    if (offset < range.start
            || offset + size > range.start + range.length
            || offset + size < offset
            || !(size == 8 || size == 16 || size == 32)) {
        cout << "IOPortRead: bad offset/size" << endl;
    }

    return portio::read(static_cast<uint16_t>(port), size);
}
