#include <syscalls/Exit.hpp>

size_t Exit(const shared_ptr<Process> &process,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t,
           uint64_t) {
    process->exit();
    cout << "Exit: Should not reach here" << endl;
    Panic();
}
