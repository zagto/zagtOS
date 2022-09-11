#include <syscalls/SubscribeInterrupt.hpp>

size_t SubscribeInterrupt(const shared_ptr<Process> &process,
                                  size_t BoundInterruptHandle,
                                  size_t eventQueueHandle,
                                  size_t eventTag,
                                  size_t,
                                  size_t) {
    auto eventQueue = process->handleManager.lookup<shared_ptr<EventQueue>>(eventQueueHandle);
    auto interrupt = process->handleManager.lookup<shared_ptr<BoundInterrupt>>(BoundInterruptHandle);
    interrupt->subscribe(eventQueue, eventTag);
    return 0;
}

