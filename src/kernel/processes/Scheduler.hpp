#pragma once

#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory>
#include <queue>

class CommonProcessor;

class Scheduler
{
private:
    /* TODO: more useful distribution of threads */
    static size_t nextProcessorID;

    Thread *idleThread;
    Thread *_activeThread{nullptr};
    threadList::List<&Thread::ownerReceptor> threads[Thread::NUM_PRIORITIES];
    Processor *processor;

    void add(Thread *thread, bool online);

    friend void dealWithException(Status);
    [[noreturn]]
    void scheduleNext();

    /* KernelEntry2 calls scheduleNext at the end */
    friend void KernelEntry2(void *);

public:
    SpinLock lock;

    Scheduler(CommonProcessor *processor, Status &status);
    ~Scheduler();
    /* new threads should be added to any scheduler in the system for even load distribution */
    static void schedule(Thread *thread, bool online);
    Status checkChanges();
    /* TODO: figure out if this is sufficient */
    void removeOtherThread(Thread *thread);
    void removeActiveThread();
    Thread *activeThread() const;
};
