#include <processes/Scheduler.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>


void Scheduler::List::append(Thread *thread) {
    assert(!thread->previous);
    assert(!thread->next);

    if (tail) {
        tail->next = thread;
        thread->previous = tail;
        tail = thread;
    } else {
        head = thread;
        tail = thread;
    }
}

void Scheduler::List::remove(Thread *thread) {
    assert(thread->previous || thread == head);
    assert(thread->next || thread == tail);
    assert(&thread->_currentProcessor->scheduler.threads[thread->currentPriority()] == this);

    if (thread == head) {
        head = thread->next;
    } else {
        thread->previous->next = thread->next;
    }
    if (thread == tail) {
        tail = thread->previous;
    } else {
        thread->next->previous = thread->previous;
    }
    thread->previous = nullptr;
    thread->next = nullptr;
}

Thread *Scheduler::List::pop() {
    assert(!empty());
    assert(!head->previous);

    Thread *result = head;
    head = head->next;
    if (!head) {
        tail = nullptr;
    }
    result->next = nullptr;
    return result;
}

bool Scheduler::List::empty() {
    return head == nullptr;
}

Scheduler::Scheduler(Processor *_processor)
{
    idleThread = new Thread(shared_ptr<Process>{},
                            VirtualAddress(reinterpret_cast<size_t>(&idleEntry)),
                            Thread::Priority::IDLE,
                            0, 0);
    processor = _processor;
    _activeThread = idleThread;
    _activeThread->_currentProcessor = processor;
    _activeThread->setState(Thread::State::Active(processor));
}

void Scheduler::add(Thread *thread) {
    // TODO: ensure current processor
    assert(thread->_currentProcessor == nullptr);
    scoped_lock sl(lock);

    thread->_currentProcessor = processor;

    if (thread->currentPriority() > _activeThread->currentPriority()) {
        /* new thread has heigher proirity - switch */
        _activeThread->setState(Thread::State::Running(processor));
        threads[_activeThread->currentPriority()].append(_activeThread);
        thread->setState(Thread::State::Active(processor));
        _activeThread = thread;
    } else {
        threads[thread->currentPriority()].append(thread);
        thread->setState(Thread::State::Running(processor));
    }
}

void Scheduler::schedule(Thread *thread) {
    // TODO
    CurrentSystem.processors[0]->scheduler.add(thread);
}

void Scheduler::remove(Thread *thread) {
    scoped_lock sl(lock);
    removeLocked(thread);
}

void Scheduler::removeLocked(Thread *thread) {
    // TODO: ensure current processor
    assert(lock.isLocked());

    if (thread == _activeThread) {
        _activeThread = {};
        scheduleNext();
    } else {
        threads[thread->currentPriority()].remove(thread);
    }
    thread->setState(Thread::State::Transition());
    thread->_currentProcessor = nullptr;
}

Thread *Scheduler::activeThread() const {
    return _activeThread;
}

void Scheduler::scheduleNext() {
    assert(!_activeThread);
    assert(lock.isLocked());

    for (ssize_t prio = Thread::NUM_PRIORITIES - 1; prio >= 0; prio--) {
        if (!threads[prio].empty()) {
            _activeThread = threads[prio].pop();
            _activeThread->setState(Thread::State::Active(processor));
            return;
        }
    }

    cout << "Help! Where is the idle thread???" << endl;
    Panic();
}

Scheduler::~Scheduler() {
    // letting processors disappear is currently not supported
    Panic();
}
