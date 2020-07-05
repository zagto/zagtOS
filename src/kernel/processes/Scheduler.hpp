#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory>
#include <queue>

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

    Thread *idleThread;
    Thread *_activeThread;
    List threads[Thread::NUM_PRIORITIES];
    Processor *processor;

    void add(Thread *thread);
    void scheduleNext();

public:
    mutex lock;

    Scheduler(Processor *processor);
    ~Scheduler();
    /* new threads should be added to any scheduler in the system for even load distribution */
    static void schedule(Thread *thread);
    void remove(Thread *thread);
    /* TODO: figure out if this is sufficient */
    void removeLocked(Thread *thread);
    Thread *activeThread() const;
};

#endif // SCHEDULER_HPP
