#include <syscalls/UnsubscribeInterrupt.hpp>

size_t UnsubscribeInterrupt(const shared_ptr<Process> &process,
                                    size_t handle,
                                    size_t,
                                    size_t,
                                    size_t,
                                    size_t) {
    auto interrupt = process->handleManager.lookupInterrupt(handle);
    interrupt->unsubscribe();
    return 0;
}

