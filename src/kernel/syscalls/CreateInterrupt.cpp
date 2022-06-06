#include <syscalls/CreateInterrupt.hpp>
#include <syscalls/UserSpaceObject.hpp>
#include <processes/InterruptManager.hpp>

struct ProcessorInterruptInfo {
    size_t processorID;
    size_t vectorNumber;
};

size_t CreateInterrupt(const shared_ptr<Process> &process,
                       size_t mode,
                       size_t numberOrResultPointer,
                       size_t triggerMode,
                       size_t,
                       size_t) {
    if (!process->canAccessPhysicalMemory()) {
        cout << "CreateInterrupt: not allowed for this process" << endl;
        throw BadUserSpace(process);
    }

    static constexpr size_t CREATE_FIXED = 1;
    static constexpr size_t CREATE_PROCESSOR_DIRECT = 2;

    if (mode == CREATE_FIXED) {
        auto interrupt = make_shared<BoundInterrupt>(InterruptType::X86_GSI,
                                                     numberOrResultPointer,
                                                     static_cast<TriggerMode>(triggerMode));
        uint32_t handle = process->handleManager.add(interrupt);
        return handle;
    } else if (mode == CREATE_PROCESSOR_DIRECT) {
        auto interrupt = make_shared<BoundInterrupt>(InterruptType::PROCESSOR_DIRECT,
                                                     0,
                                                     static_cast<TriggerMode>(triggerMode));
        UserSpaceObject<ProcessorInterruptInfo, USOOperation::WRITE> uso(numberOrResultPointer);
        uso.object.processorID = interrupt->processorInterrupt().processorID;
        uso.object.vectorNumber = interrupt->processorInterrupt().vectorNumber;
        uso.writeOut();
        return process->handleManager.add(interrupt);
    } else {
        cout << "CreateInterrupt: invalid mode number" << endl;
        throw BadUserSpace(process);
    }
}
