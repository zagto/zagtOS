#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <lib/List.hpp>

#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>

class Scheduler
{
private:
    Thread *idleThread;
    Thread *_currentThread{nullptr};
    List<Thread> threads[Thread::NUM_PRIORITIES];

public:
    Scheduler(Processor *processor);

    Thread *currentThread();
    void add(Thread *thread);
    void remove(Thread *thread);
    void scheduleNext();
};

#endif // SCHEDULER_HPP
