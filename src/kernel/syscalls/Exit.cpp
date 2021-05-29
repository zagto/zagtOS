#include <syscalls/Exit.hpp>

Result<size_t> Exit(const shared_ptr<Process> &process,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t) {
    Status status = process->exit();
    assert(status == Status::ThreadKilled());
    return status;
}
