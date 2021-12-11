#include <syscalls/CreatePort.hpp>
#include <processes/Port.hpp>

size_t CreatePort(const shared_ptr<Process> &process,
                          uint64_t,
                          uint64_t,
                          uint64_t,
                          uint64_t,
                          uint64_t) {
    shared_ptr<Port> port = make_shared<Port>(process);
    uint32_t handle = process->handleManager.addPort(port);
    return handle;
}
