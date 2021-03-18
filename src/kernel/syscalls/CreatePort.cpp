#include <syscalls/CreatePort.hpp>
#include <processes/Port.hpp>

Result<size_t> CreatePort(const shared_ptr<Process> &process,
                          uint64_t,
                          uint64_t,
                          uint64_t,
                          uint64_t,
                          uint64_t) {
    Result<shared_ptr<Port>> port = make_shared<Port>(process);
    if (!port) {
        return port.status();
    }
    Result handle = process->handleManager.addPort(*port);
    if (handle) {
        return *handle;
    } else {
        return handle.status();
    }
}
