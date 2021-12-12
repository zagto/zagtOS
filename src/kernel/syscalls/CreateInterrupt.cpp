#include <syscalls/CreateInterrupt.hpp>
#include <processes/InterruptManager.hpp>

size_t CreateInterrupt(const shared_ptr<Process> &process,
                       size_t,
                       size_t,
                       size_t,
                       size_t,
                       size_t) {
    if (!process->canAccessPhysicalMemory()) {
        cout << "CreateInterrupt: not allowed for this process" << endl;
        throw BadUserSpace(process);
    }

    auto interrupt = make_shared<ProcessInterrupt>();
    uint32_t handle = process->handleManager.addInterrupt(interrupt);
    return handle;
}
