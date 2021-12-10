#include <processes/Scheduler.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <processes/KernelThreadEntry.hpp>
#include <system/Processor.hpp>


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
    idleThread->currentProcessor(processor);
}

void Scheduler::add(Thread *thread, bool online) {
    // TODO: ensure current processor
    assert(thread->currentProcessor() == nullptr);

    scoped_lock sl1(KernelInterruptsLock);
    scoped_lock sl(lock);
    if (online) {
        assert(processor->activeThread() != nullptr);
    }

    cout << "add thread " << thread << " to scheduler " << processor->id << endl;

    thread->currentProcessor(processor);

    if (online && thread->currentPriority() > processor->activeThread()->currentPriority()) {
        /* new thread has heigher proirity - switch */
        /*if (CurrentProcessor == processor) {
            _activeThread->setState(Thread::State::Running(processor));
            threads[_activeThread->currentPriority()].append(_activeThread);
            thread->setState(Thread::State::Active(processor));
            _activeThread = thread;
        } else {*/
            threads[thread->currentPriority()].append(thread);
            thread->setState(Thread::State::Running(processor));
            processor->sendIPI(IPI::CheckScheduler);
       //}
    } else {
        threads[thread->currentPriority()].append(thread);
        thread->setState(Thread::State::Running(processor));
    }
}

Status Scheduler::checkChanges() {
    lock.lock();

    Thread *activeThread = processor->activeThread();

    for (size_t prio = Thread::NUM_PRIORITIES-1; prio > activeThread->currentPriority(); prio--) {
        if (!threads[prio].empty()) {
            activeThread->setState(Thread::State::Running(processor));
            threads[activeThread->currentPriority()].append(activeThread);
            processor->activeThread(nullptr);

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
    assert(thread != processor->activeThread());

    threads[thread->currentPriority()].remove(thread);
    thread->setState(Thread::State::Transition());
    thread->currentProcessor(nullptr);
}

void Scheduler::removeActiveThread() {
    assert(CurrentProcessor() == processor);
    Thread *thread = CurrentThread();
    assert(thread == processor->activeThread());

    thread->setState(Thread::State::Transition());
    thread->currentProcessor(nullptr);
    processor->activeThread(nullptr);
}

[[noreturn]]
void Scheduler::scheduleNext() {
    assert(!processor->activeThread());
    assert(lock.isLocked());
    assert(processor == CurrentProcessor());

    for (ssize_t prio = Thread::NUM_PRIORITIES - 1; prio >= 0; prio--) {
        if (!threads[prio].empty()) {
            Thread *newActiveThread = threads[prio].pop();
            processor->activeThread(newActiveThread);
            newActiveThread->setState(Thread::State::Active(processor));

            if (newActiveThread == idleThread) {
                cout << "activeThread on " << processor->id << " is our idle Thread" << endl;
            } else {
                cout << "activeThread on " << processor->id << " is now ";
                for (char c: newActiveThread->process->logName) {
                    cout << c;
                }
                cout <<":" << newActiveThread->handle() << " (" << newActiveThread << ")" << endl;
            }

            assert(newActiveThread->currentProcessor() == processor);
            newActiveThread->kernelStack->switchToKernelEntry(newActiveThread->kernelEntry, newActiveThread->kernelEntryData);
        }
    }

    cout << "Help! Where is the idle thread???" << endl;
    Panic();
}

Scheduler::~Scheduler() {
    // letting processors disappear is currently not supported
    Panic();
}
