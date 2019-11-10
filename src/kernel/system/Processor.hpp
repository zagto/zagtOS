#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <processes/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>
#include <system/KernelStack.hpp>

class Process;
class PagingContext;

class Processor {
public:
    /* kernelStack has to be the first field, context switch code relies on this */
    KernelStack *kernelStack;

protected:
    friend class Logger;

    static const size_t LOG_BUFFER_SIZE = 500;
    char logBuffer[LOG_BUFFER_SIZE];
    size_t logBufferIndex;

public:
    Scheduler scheduler;
    Interrupts interrupts;
    Process *currentProcess{nullptr};
    PagingContext *activePagingContext;

    Processor(bool bootProcessor);
    ~Processor();
};

#endif // PROCESSOR_HPP
