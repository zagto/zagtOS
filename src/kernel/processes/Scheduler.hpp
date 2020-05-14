#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <processes/Thread.hpp>
#include <interrupts/RegisterState.hpp>
#include <memory>
#include <queue>

class Scheduler
{
private:
    shared_ptr<Thread> idleThread;
    shared_ptr<Thread> _currentThread{};
    queue<weak_ptr<Thread>> threads[Thread::NUM_PRIORITIES];

public:
    Scheduler(Processor *processor);
    ~Scheduler();

    shared_ptr<Thread> currentThread();
    void add(shared_ptr<Thread> thread);
    void remove(shared_ptr<Thread> thread);
    void scheduleNext();
};

#endif // SCHEDULER_HPP
