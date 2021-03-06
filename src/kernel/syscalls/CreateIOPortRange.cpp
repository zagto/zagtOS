#include <syscalls/CreateIOPortRange.hpp>
#include <processes/HandleManager.hpp>

size_t CreateIOPortRange(const shared_ptr<Process> &process,
                                 size_t start,
                                 size_t max,
                                 size_t,
                                 size_t,
                                 size_t) {
    if (start > 0xffff || max > 0xffff || max < start) {
        cout << "CreateIOPortRange: bad range given" << endl;
        throw BadUserSpace(process);
    }
    if (!process->canAccessPhysicalMemory()) {
        cout << "CreateIOPortRange: called from process that cannot access physical hardware"
             << endl;
        throw BadUserSpace(process);
    }

    return process->handleManager.add(IOPortRange{static_cast<uint16_t>(start),
                                                     static_cast<uint16_t>(max)});
}

