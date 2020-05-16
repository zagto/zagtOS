#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory>
#include <queue>

class Scheduler
{
private:
    class List {
        shared_ptr<Thread> head;
        shared_ptr<Thread> tail;
    };

    shared_ptr<Thread> idleThread;
    shared_ptr<Thread> _currentThread;
    List threads[Thread::NUM_PRIORITIES];

    void add(shared_ptr<Thread> thread);

public:
    mutex lock;

    Scheduler(Processor *processor);
    ~Scheduler();
    /* new threads should be added to any scheduler in the system for even load distribution */
    static void schedule(shared_ptr<Thread> thread);
    void remove(shared_ptr<Thread> thread);
    void scheduleNext();
};

#endif // SCHEDULER_HPP
