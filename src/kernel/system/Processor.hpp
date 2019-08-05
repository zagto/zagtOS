#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <tasks/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>
#include <system/KernelStack.hpp>

class Task;
class MasterPageTable;

class Processor {
    public:
        KernelStack *kernelStack;
        Scheduler scheduler;
        Interrupts interrupts;
        Task *currentTask{nullptr};
        MasterPageTable *activeMasterPageTable;

        Processor(bool bootProcessor);
        ~Processor();
};

#endif // PROCESSOR_HPP
