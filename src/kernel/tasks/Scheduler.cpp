#include <tasks/Scheduler.hpp>
#include <interrupts/util.hpp>


Scheduler::Scheduler(Processor *processor)
{
    idleThread = new Thread(nullptr,
                            VirtualAddress(reinterpret_cast<usize>(&idleEntry)),
                            Thread::Priority::IDLE,
                            0, THREAD_STRUCT_AREA_SIZE, 0, 0);
    _currentThread = idleThread;
    _currentThread->currentProcessor = processor;
}


Thread *Scheduler::currentThread() {
    return _currentThread;
}


void Scheduler::add(Thread *thread) {
    if (thread->currentPriority > _currentThread->currentPriority) {

        thread->currentProcessor = _currentThread->currentProcessor;
        _currentThread->currentProcessor = nullptr;
        threads[_currentThread->currentPriority].pushBack(_currentThread);
        _currentThread = thread;
    } else {
        threads[thread->currentPriority].pushBack(thread);
    }
}

void Scheduler::remove(Thread *thread) {
    if (thread == _currentThread) {
        _currentThread = nullptr;
        scheduleNext();
    } else {
        threads[thread->currentPriority].remove(thread);
    }
}

void Scheduler::scheduleNext() {
    Assert(_currentThread == nullptr);

    for (isize prio = Thread::NUM_PRIORITIES; prio >= 0; prio--) {
        if (!threads[prio].isEmpty()) {
            _currentThread = threads[prio].popFront();
            return;
        }
    }

    Log << "Help! Where is the idle thread???" << EndLine;
    Panic();
}
