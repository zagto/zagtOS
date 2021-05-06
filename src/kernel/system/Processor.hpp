#pragma once

#include <processes/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>
#include <memory/InvalidateQueue.hpp>

class Process;
class TLBContext;

class Processor {
public:
    /* kernelStack has to be the first field, context switch code relies on this */
    uint8_t *kernelStack;

    static const size_t LOG_BUFFER_SIZE = 500;
    char logBuffer[LOG_BUFFER_SIZE];
    size_t logBufferIndex;

protected:
    friend class TLBContext;
    SpinLock tlbContextsLock;
    uint16_t nextLocalTlbContextID;
    InvalidateQueue invalidateQueue;

public:
    const size_t id;
    Scheduler scheduler;
    Interrupts interrupts;
    size_t activeTLBContextID;

    void sendInvalidateQueueProcessingIPI();

    Processor(size_t id, Status &status);
    ~Processor();
};

extern Processor *Processors;
