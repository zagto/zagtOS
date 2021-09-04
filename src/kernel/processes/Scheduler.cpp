#include <processes/Scheduler.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <processes/KernelThreadEntry.hpp>
#include <system/Processor.hpp>


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
    assert(&thread->currentProcessor()->scheduler.threads[thread->currentPriority()] == this);

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
    } else {
        head->previous = nullptr;
    }
    result->next = nullptr;
    return result;
}

bool Scheduler::List::empty() {
    return head == nullptr;
}

Scheduler::Scheduler(CommonProcessor *_processor, Status &status)
{
    if (!status) {
        return;
    }

    Result result = make_raw<Thread>(IdleThreadEntry,
                                     Thread::Priority::IDLE);
    if (!result) {
        status = result.status();
        return;
    }
    idleThread = *result;
    threads[Thread::Priority::IDLE].append(idleThread);

    cout << "created idle thread at " << idleThread << " to scheduler " << _processor->id << endl;

    processor = static_cast<Processor *>(_processor);
    idleThread->setState(Thread::State::Running(processor));
}

void Scheduler::add(Thread *thread, bool online) {
    // TODO: ensure current processor
    assert(thread->currentProcessor() == nullptr);
    if (online) {
        Processor::kernelInterruptsLock.lock();
    }

    scoped_lock sl(lock);
    if (online) {
        assert(_activeThread != nullptr);
    }

    cout << "add thread " << thread << " to scheduler " << processor->id << endl;

    thread->currentProcessor(processor);

    if (online && thread->currentPriority() > _activeThread->currentPriority()) {
        /* new thread has heigher proirity - switch */
        /*if (CurrentProcessor == processor) {
            _activeThread->setState(Thread::State::Running(processor));
            threads[_activeThread->currentPriority()].append(_activeThread);
            thread->setState(Thread::State::Active(processor));
            _activeThread = thread;
        } else {*/
            threads[thread->currentPriority()].append(thread);
            thread->setState(Thread::State::Running(processor));
            processor->sendCheckSchedulerIPI();
       //}
    } else {
        threads[thread->currentPriority()].append(thread);
        thread->setState(Thread::State::Running(processor));
    }

    if (online) {
        Processor::kernelInterruptsLock.unlock();
    }
}

Status Scheduler::checkChanges() {
    lock.lock();

    for (size_t prio = Thread::NUM_PRIORITIES-1; prio > _activeThread->currentPriority(); prio--) {
        if (!threads[prio].empty()) {
            _activeThread->setState(Thread::State::Running(processor));
            threads[_activeThread->currentPriority()].append(_activeThread);
            _activeThread = {};

            return Status::DiscardStateAndSchedule();
        }
    }

    lock.unlock();
    return Status::OK();
}

size_t Scheduler::nextProcessorID{0};

void Scheduler::schedule(Thread *thread, bool online) {
    // TODO
    size_t processorID = __atomic_fetch_add(&nextProcessorID, 1, __ATOMIC_RELAXED) % CurrentSystem.numProcessors;
    Processors[processorID].scheduler.add(thread, online);
}

void Scheduler::removeOtherThread(Thread *thread) {
    assert(lock.isLocked());
    assert(thread != _activeThread);

    threads[thread->currentPriority()].remove(thread);
    thread->setState(Thread::State::Transition());
    thread->currentProcessor(nullptr);
}

void Scheduler::removeActiveThread() {
    assert(CurrentProcessor == processor);
    assert(CurrentThread() == _activeThread);

    _activeThread->setState(Thread::State::Transition());
    _activeThread->currentProcessor(nullptr);
    _activeThread = nullptr;
}

Thread *Scheduler::activeThread() const {
    return _activeThread;
}

[[noreturn]]
void Scheduler::scheduleNext() {
    assert(!_activeThread);
    assert(lock.isLocked());

    for (ssize_t prio = Thread::NUM_PRIORITIES - 1; prio >= 0; prio--) {
        if (!threads[prio].empty()) {
            _activeThread = threads[prio].pop();
            _activeThread->setState(Thread::State::Active(processor));

            if (_activeThread == idleThread) {
                cout << "activeThread on " << processor->id << " is our idle Thread" << endl;
            } else {
                cout << "activeThread on " << processor->id << " is now ";
                for (char c: _activeThread->process->logName) {
                    cout << c;
                }
                cout <<":" << _activeThread->handle() << " (" << _activeThread << ")" << endl;
            }

            _activeThread->kernelStack->switchToKernelEntry(_activeThread->kernelEntry, _activeThread->kernelEntryData);
        }
    }

    cout << "Help! Where is the idle thread???" << endl;
    Panic();
}

Scheduler::~Scheduler() {
    // letting processors disappear is currently not supported
    Panic();
}
