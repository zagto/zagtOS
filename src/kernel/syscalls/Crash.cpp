#include <syscalls/Crash.hpp>

size_t Crash(const shared_ptr<Process> &process,
                     size_t,
                     size_t,
                     size_t,
                     size_t,
                     size_t) {
    throw BadUserSpace(process);
}
