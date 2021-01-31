#include <system/Processor.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>

extern size_t KERNEL_STACK_SIZE;

Processor::Processor(bool bootProcessor) :
        logBufferIndex{0},
        scheduler(this),
        interrupts(bootProcessor),
        activePagingContext{nullptr} {
    kernelStack = CurrentSystem.memory.allocateVirtualArea(KERNEL_STACK_SIZE, 16).asPointer<uint8_t>();
}

Processor::~Processor() {
    // letting processors disappear is currently not supported
    Panic();
}
