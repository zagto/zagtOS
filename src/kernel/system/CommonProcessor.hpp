#pragma once

#include <processes/Scheduler.hpp>
#include <interrupts/Interrupts.hpp>
#include <memory/InvalidateQueue.hpp>
#include <memory/KernelPageAllocator.hpp>
#include <processes/KernelThreadEntry.hpp>
#include <memory>

class Process;
class TLBContext;
class PagingContext;
class Processor;

class CommonProcessor {
public:
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
    friend class KernelInterruptsLockClass;
    friend void _handleInterrupt(RegisterState *);
    friend void InKernelReturnEntryRestoreInterruptsLock(RegisterState *);
    size_t interruptsLockValue{1};
    uint32_t ipiFlags;

public:
    InvalidateQueue invalidateQueue;
    const size_t id;
    Scheduler scheduler;

    /* For KernelPageAllocator::invalidateQueue */
    KernelVirtualAddress kernelInvalidateProcessedUntil;

    TLBContextID activatePagingContext(PagingContext *pagingContext, TLBContextID tryFirst);

    CommonProcessor(size_t id, Status &status);
    CommonProcessor(CommonProcessor &) = delete;
    CommonProcessor &operator=(CommonProcessor &) = delete;
    ~CommonProcessor();
};

extern Processor *Processors;
