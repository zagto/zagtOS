#include <system/Processor.hpp>
#include <paging/MasterPageTable.hpp>
#include <system/System.hpp>


Processor::Processor(bool bootProcessor) :
        scheduler(this),
        interrupts(bootProcessor),
        activeMasterPageTable{nullptr} {
    kernelStack = new KernelStack;
}

Processor::~Processor() {
    // letting processors disappear is currently not supported
    Panic();
}
