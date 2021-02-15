#include <system/Processor.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>

extern size_t KERNEL_STACK_SIZE;

Processor::Processor(bool bootProcessor, Status &status) :
        logBufferIndex{0},
        scheduler(this),
        interrupts(bootProcessor),
        activePagingContext{nullptr} {

    Result<KernelVirtualAddress> address = CurrentSystem.memory.allocateVirtualArea(KERNEL_STACK_SIZE, 16);
    if (address) {
        kernelStack = address->asPointer<uint8_t>();
        status = Status::OK();
    } else {
        kernelStack = nullptr;
        status = address.status();
    }
}

Processor::~Processor() {
    // letting processors disappear is currently not supported
    Panic();
}
