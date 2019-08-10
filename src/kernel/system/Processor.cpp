#include <system/Processor.hpp>
#include <paging/PagingContext.hpp>
#include <system/CommonSystem.hpp>


Processor::Processor(bool bootProcessor) :
        scheduler(this),
        interrupts(bootProcessor),
        activePagingContext{nullptr} {
    kernelStack = new KernelStack;
}

Processor::~Processor() {
    // letting processors disappear is currently not supported
    Panic();
}
