#include <syscalls/ProcessedInterrupt.hpp>

size_t ProcessedInterrupt(const shared_ptr<Process> &process,
                                  size_t handle,
                                  size_t,
                                  size_t,
                                  size_t,
                                  size_t) {
    auto interrupt = process->handleManager.lookupInterrupt(handle);
    interrupt->processed();
    return 0;
}

