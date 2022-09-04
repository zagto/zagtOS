#pragma once

#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <time/Time.hpp>
#include <memory>
#include <queue>

class CommonProcessor;

class Scheduler
{
private:
    /* TODO: more useful distribution of threads */
    static size_t nextProcessorID;

    Thread *idleThread;
    threadList::List<&Thread::ownerReceptor> readyThreads[Thread::NUM_PRIORITIES];
    threadList::List<&Thread::ownerReceptor> timerThreads[ClockID::COUNT];
    Processor *processor;

    void addUnlocked(Thread *thread, bool online) noexcept;
    void add(Thread *thread, bool online) noexcept;
    void updateTimer() noexcept;

public:
    SpinLock lock;

    Scheduler(CommonProcessor *processor);
    ~Scheduler();

    /* new threads should be added to any scheduler in the system for even load distribution */
    static void schedule(Thread *thread, bool online) noexcept;
    void checkChanges();
    /* Makes the current Thread wait for a timer, then returns to user space */
    [[noreturn]] void setTimer(ClockID clockID, uint64_t timestamp);
    /* TODO: figure out if this is sufficient */
    void removeOtherThread(Thread *thread) noexcept;
    void removeActiveThread();
    /* Don't use this normally, throw an Exception instead. This function is used in special
     * places, like where these exceptions are handled. */
    [[noreturn]] void scheduleNext() noexcept;
};
