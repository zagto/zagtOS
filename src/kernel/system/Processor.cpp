#include <system/Processor.hpp>
#include <paging/MasterPageTable.hpp>
#include <system/System.hpp>


Processor::Processor() :
        scheduler(this),
        activeMasterPageTable{nullptr} {
    kernelStack = new KernelStack;
}

Processor::~Processor() {
    // letting processors disappear is currently not supported
    Panic();
}
