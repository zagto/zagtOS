#include <syscalls/WaitInterrupt.hpp>

size_t WaitInterrupt(const shared_ptr<Process> &process,
                             size_t handle,
                             size_t,
                             size_t,
                             size_t,
                             size_t) {
    auto interrupt = process->handleManager.lookupInterrupt(handle);
    interrupt->platformInterrupt.wait(*interrupt);
    return 0;
}

