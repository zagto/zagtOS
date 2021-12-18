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

    if (offset < range.start
            || offset + size > range.start + range.length
            || offset + size < offset
            || !(size == 8 || size == 16 || size == 32)) {
        cout << "IOPortWrite: bad offset/size" << endl;
    }

    portio::write(static_cast<uint16_t>(port), size, value);
    return 0;
}

