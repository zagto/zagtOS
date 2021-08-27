#pragma once

#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory>
#include <queue>

class CommonProcessor;

class Scheduler
{
private:
    struct List {
        Thread *head{nullptr};
        Thread *tail{nullptr};

        void append(Thread *thread);
        void remove(Thread *thread);
        Thread *pop();
        bool empty();
    };

    /* TODO: more useful distribution of threads */
    static size_t nextProcessorID;

    Thread *idleThread;
    Thread *_activeThread{nullptr};
    List threads[Thread::NUM_PRIORITIES];
    Processor *processor;

    void add(Thread *thread, bool online);
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
    void remove(Thread *thread);
    /* TODO: figure out if this is sufficient */
    void removeLocked(Thread *thread);
    Thread *activeThread() const;
};
