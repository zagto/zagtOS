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
protected:
    /* self variable since the pointer to the current Processor may be saved in a register (gs on
     * x86) that is not easily readable */
    CommonProcessor *self;

    /* activeThread is mostly saved here instead of the scheduler for easy access by ContextSwitch.
       Please hold Scheduler->lock when reading this variable form remote processors. */
    Thread *_activeThread{nullptr};

    /* to quickly find the stack in ContextSwitch. Updated when setting activeThread. */
    RegisterState *userRegisterState;

    friend class Logger;
    static const size_t LOG_BUFFER_SIZE = 500;
    char logBuffer[LOG_BUFFER_SIZE];
    size_t logBufferIndex;

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
    shared_ptr<KernelStack> kernelStack;

    size_t hardwareID;

    InvalidateQueue invalidateQueue;
    const size_t id;
    Scheduler scheduler;

    /* For KernelPageAllocator::invalidateQueue */
    KernelVirtualAddress kernelInvalidateProcessedUntil;

    TLBContextID activatePagingContext(PagingContext *pagingContext, TLBContextID tryFirst) noexcept;

    CommonProcessor();
    CommonProcessor(CommonProcessor &) = delete;
    CommonProcessor &operator=(CommonProcessor &) = delete;
    ~CommonProcessor();

    Thread *activeThread() const noexcept;
    void activeThread(Thread *) noexcept;
};

extern Processor *Processors;
