#include <syscalls/GetFirmwareRoot.hpp>
#include <system/System.hpp>

size_t GetFirmwareRoot(const shared_ptr<Process> &process,
                               size_t,
                               size_t,
                               size_t,
                               size_t,
                               size_t) {
    if (process->canAccessPhysicalMemory()) {
        return CurrentSystem.ACPIRoot.value();
    } else {
        throw BadUserSpace(process);
    }
}
