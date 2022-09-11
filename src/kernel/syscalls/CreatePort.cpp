#include <syscalls/CreatePort.hpp>
#include <processes/HandleManager.hpp>

size_t CreatePort(const shared_ptr<Process> &process,
                          size_t eventQueueHandle,
                          size_t eventTag,
                          size_t,
                          size_t,
                          size_t) {
    auto eventQueue = process->handleManager.lookup<shared_ptr<EventQueue>>(eventQueueHandle);
    shared_ptr<Port> port = make_shared<Port>(eventQueue, eventTag);
    uint32_t handle = process->handleManager.add(port);
    return handle;
}
