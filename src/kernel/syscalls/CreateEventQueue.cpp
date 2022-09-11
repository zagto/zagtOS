#include <syscalls/CreateEventQueue.hpp>
#include <processes/EventQueue.hpp>

size_t CreateEventQueue(const shared_ptr<Process> &process,
                        size_t,
                        size_t,
                        size_t,
                        size_t,
                        size_t) {
    auto eventQueue = make_shared<EventQueue>(process);
    uint32_t handle = process->handleManager.add(eventQueue);
    return handle;
}
