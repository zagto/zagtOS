#include <syscalls/Exit.hpp>

Result<size_t> Exit(const shared_ptr<Process> &process,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t) {
    process->exit();
    return 0;
}
