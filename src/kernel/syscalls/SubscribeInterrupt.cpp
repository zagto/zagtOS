#include <syscalls/SubscribeInterrupt.hpp>

size_t SubscribeInterrupt(const shared_ptr<Process> &process,
                                  size_t handle,
                                  size_t,
                                  size_t,
                                  size_t,
                                  size_t) {
    auto interrupt = process->handleManager.lookup<shared_ptr<BoundInterrupt>>(handle);
    interrupt->subscribe();
    return 0;
}

