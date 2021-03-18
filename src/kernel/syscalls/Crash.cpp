#include <syscalls/Crash.hpp>

Result<size_t> Crash(const shared_ptr<Process> &,
            uint64_t,
            uint64_t,
            uint64_t,
            uint64_t,
            uint64_t) {
    return Status::BadUserSpace();
}
