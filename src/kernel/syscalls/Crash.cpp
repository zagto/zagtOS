#include <syscalls/Crash.hpp>

Result<size_t> Crash(const shared_ptr<Process> &,
                     size_t,
                     size_t,
                     size_t,
                     size_t,
                     size_t) {
    return Status::BadUserSpace();
}
