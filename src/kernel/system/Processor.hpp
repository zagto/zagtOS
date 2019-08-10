#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <tasks/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>
#include <system/KernelStack.hpp>

class Task;
class PagingContext;

class Processor {
    public:
        KernelStack *kernelStack;
        Scheduler scheduler;
        Interrupts interrupts;
        Task *currentTask{nullptr};
        PagingContext *activePagingContext;

        Processor(bool bootProcessor);
        ~Processor();
};

#endif // PROCESSOR_HPP
