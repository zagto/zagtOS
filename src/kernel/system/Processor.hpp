#pragma once

#include <processes/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>
#include <memory/InvalidateQueue.hpp>

class Process;
class TLBContext;
class PagingContext;

class Processor {
public:
    class KernelInterruptsLock {
    private:
        KernelInterruptsLock(const KernelInterruptsLock &) = delete;
        void operator=(const KernelInterruptsLock &) = delete;

    public:
        void lock();
        void unlock();
        bool isLocked() const;
    };

    /* kernelStack has to be the first field, context switch code relies on this */
    uint8_t *kernelStack;

    static const size_t LOG_BUFFER_SIZE = 500;
    char logBuffer[LOG_BUFFER_SIZE];
    size_t logBufferIndex;

protected:
    friend class TLBContext;
    friend class PageOutContext;
    SpinLock tlbContextsLock;
    TLBContextID nextTlbContextID();
    TLBContextID activeTLBContextID;

    friend class SpinLock;
    friend class KernelInterruptsLock;
    bool interruptsLockLocked{true};

public:
    /* has to be static - the actual current processor can only be determined once this is
     * locked. */
    static KernelInterruptsLock kernelInterruptsLock;

    InvalidateQueue invalidateQueue;
    const size_t id;
    Scheduler scheduler;
    Interrupts interrupts;

    void sendInvalidateQueueProcessingIPI();
    TLBContextID activatePagingContext(PagingContext *pagingContext, TLBContextID tryFirst);

    Processor(size_t id, Status &status);
    ~Processor();
};

extern Processor *Processors;
