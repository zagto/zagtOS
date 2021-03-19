#pragma once

#include <processes/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>

class Process;
class PagingContext;

class Processor {
public:
    /* kernelStack has to be the first field, context switch code relies on this */
    uint8_t *kernelStack;

    static const size_t LOG_BUFFER_SIZE = 500;
    char logBuffer[LOG_BUFFER_SIZE];
    size_t logBufferIndex;

public:
    Scheduler scheduler;
    Interrupts interrupts;
    PagingContext *activePagingContext;

    Processor(bool bootProcessor, Status &status);
    ~Processor();
};
