#include <syscalls/CreateInterrupt.hpp>
#include <processes/InterruptManager.hpp>

size_t CreateInterrupt(const shared_ptr<Process> &process,
                       size_t mode,
                       size_t numberOrResultPointer,
                       size_t triggerMode,
                       size_t polarity,
                       size_t) {
    if (!process->canAccessPhysicalMemory()) {
        cout << "CreateInterrupt: not allowed for this process" << endl;
        throw BadUserSpace(process);
    }

    static constexpr size_t CREATE_FIXED = 1;
    static constexpr size_t CREATE_ANY = 2;

    if (mode == CREATE_FIXED) {
        auto interrupt = make_shared<BoundInterrupt>(InterruptType::X86_GSI,
                                                     numberOrResultPointer,
                                                     static_cast<TriggerMode>(triggerMode),
                                                     static_cast<Polarity>(polarity));
        uint32_t handle = process->handleManager.add(interrupt);
        return handle;
    } else if (mode == CREATE_ANY) {
        cout << "TODO" << endl;
        Panic();
    } else {
        cout << "CreateInterrupt: invalid mode number" << endl;
        throw BadUserSpace(process);
    }
}
