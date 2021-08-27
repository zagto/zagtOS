#pragma once

#include <processes/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>
#include <memory/InvalidateQueue.hpp>
#include <memory>

class Process;
class TLBContext;
class PagingContext;
class Processor;

class CommonProcessor {
public:
    class KernelInterruptsLock {
    public:
        KernelInterruptsLock() {}
        KernelInterruptsLock(const KernelInterruptsLock &) = delete;
        void operator=(const KernelInterruptsLock &) = delete;

        void lock();
        void unlock();
        bool isLocked() const;
    };

    shared_ptr<KernelStack> kernelStack;

    size_t hardwareID;

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

    void sendInvalidateQueueProcessingIPI();
    TLBContextID activatePagingContext(PagingContext *pagingContext, TLBContextID tryFirst);

    CommonProcessor(size_t id, Status &status);
    CommonProcessor(CommonProcessor &) = delete;
    CommonProcessor &operator=(CommonProcessor &) = delete;
    ~CommonProcessor();
};

extern Processor *Processors;
