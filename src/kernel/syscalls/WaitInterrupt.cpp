#include <syscalls/WaitInterrupt.hpp>

size_t WaitInterrupt(const shared_ptr<Process> &process,
                             size_t handle,
                             size_t,
                             size_t,
                             size_t,
                             size_t) {
    auto interrupt = process->handleManager.lookup<shared_ptr<BoundInterrupt>>(handle);
    interrupt->wait();
    return WAIT_INTERRUPT_SUCCESS;
}

