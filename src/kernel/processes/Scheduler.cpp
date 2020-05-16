#include <processes/Scheduler.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>


Scheduler::Scheduler(Processor *processor)
{
    idleThread = make_shared<Thread>(shared_ptr<Process>{},
                                     VirtualAddress(reinterpret_cast<size_t>(&idleEntry)),
                                     Thread::Priority::IDLE,
                                     0,
                                     0,
                                     0,
                                     0);
    _currentThread = idleThread;
    _currentThread->currentProcessor = processor;
}



void Scheduler::add(shared_ptr<Thread> thread) {
    shared_ptr<Thread> current = _currentThread;

    if (thread->currentPriority > current->currentPriority) {
        thread->currentProcessor = current->currentProcessor;
        current->currentProcessor = nullptr;
        threads[current->currentPriority].push_back(_currentThread);
        _currentThread = thread;
    } else {
        threads[thread->currentPriority].push_back(thread);
    }
}

void Scheduler::schedule(shared_ptr<Thread> thread) {
    // TODO
    CurrentSystem.processors[0]->scheduler.add(thread);
}

void Scheduler::remove(shared_ptr<Thread> thread) {
    if (thread == currentThread()) {
        _currentThread = {};
        thread->currentProcessor = nullptr;
        scheduleNext();
    } else {
        threads[thread->currentPriority].remove(thread);
    }
}

void Scheduler::scheduleNext() {
    assert(!_currentThread);

    for (ssize_t prio = Thread::NUM_PRIORITIES - 1; prio >= 0; prio--) {
        while (!_currentThread) {
            if (threads[prio].empty()) {
                goto out;
            }
            _currentThread = threads[prio].top().lock();
            threads[prio].pop();
        }
        currentThread()->currentProcessor = CurrentProcessor;
        return;
    out:;
    }

    cout << "Help! Where is the idle thread???" << endl;
    Panic();
}

Scheduler::~Scheduler() {
    // letting processors disappear is currently not supported
    Panic();
}
