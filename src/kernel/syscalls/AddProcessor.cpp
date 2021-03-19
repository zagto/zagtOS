#include <syscalls/AddProcessor.hpp>
#include <system/System.hpp>

Result<size_t> AddProcessor(const shared_ptr<Process> &process,
                            size_t hardwareID,
                            size_t,
                            size_t,
                            size_t,
                            size_t) {
    if (!process->canAccessPhysicalMemory()) {
        return Status::BadUserSpace();
    }

    CurrentProcessor->interrupts.wakeSecondaryProcessor(hardwareID);
    return 0;
}

