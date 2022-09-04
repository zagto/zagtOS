#include <processes/Scheduler.hpp>
#include <system/System.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <processes/KernelThreadEntry.hpp>
#include <system/Processor.hpp>
#include <lib/Exception.hpp>


Scheduler::Scheduler(CommonProcessor *_processor)
{
    idleThread = new Thread(IdleThreadEntry, Thread::Priority::IDLE);
    readyThreads[Thread::Priority::IDLE].append(idleThread);

    processor = static_cast<Processor *>(_processor);
    idleThread->setState(Thread::State::Ready(processor));
    idleThread->currentProcessor(processor);
}

void Scheduler::addUnlocked(Thread *thread, bool online) noexcept {
    // TODO: ensure current processor
    assert(thread->currentProcessor() == nullptr);

    if (online) {
        assert(processor->activeThread() != nullptr);
    }

    thread->currentProcessor(processor);

    if (online && thread->currentPriority() > processor->activeThread()->currentPriority()) {
        readyThreads[thread->currentPriority()].append(thread);
        thread->setState(Thread::State::Ready(processor));
        processor->sendIPI(IPI::CheckScheduler);
    } else {
        readyThreads[thread->currentPriority()].append(thread);
        thread->setState(Thread::State::Ready(processor));
    }
}

void Scheduler::add(Thread *thread, bool online) noexcept {
    scoped_lock sl1(KernelInterruptsLock);
    scoped_lock sl(lock);
    addUnlocked(thread, online);
}

void Scheduler::checkChanges() {
    /* should already be locked here - but hold it a second time for DiscardStateAndSchedule */
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    Thread *activeThread = processor->activeThread();

    for (size_t clockID = 0; clockID < ClockID::COUNT; clockID++) {
        uint64_t now = CurrentSystem.time.getClockValue(clockID);
        while (!timerThreads[clockID].empty()
               && timerThreads[clockID].begin()->state().time() <= now) {
            Thread *thread = timerThreads[clockID].pop();
            addUnlocked(thread, true);
            cout << "woke up thread " << thread << " waiting for timer" << endl;
        }
    }

    for (size_t prio = Thread::NUM_PRIORITIES-1; prio > activeThread->currentPriority(); prio--) {
        if (!readyThreads[prio].empty()) {
            activeThread->setState(Thread::State::Ready(processor));
            readyThreads[activeThread->currentPriority()].append(activeThread);
            processor->activeThread(nullptr);

            throw DiscardStateAndSchedule(activeThread, move(sl), move(sl2));
        }
    }
}

[[noreturn]] void Scheduler::setTimer(ClockID clockID, uint64_t timestamp)  {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    cout << "setTimer" << timestamp << endl;

    Thread *activeThread = processor->activeThread();

    removeActiveThread();

    threadList::Iterator<&Thread::ownerReceptor> it;
    for (it = timerThreads->begin(); it != timerThreads->end(); ++it) {
        if (it->state().time() < timestamp) {
            break;
        }
    }
    if (it == timerThreads[clockID].end()) {
        timerThreads[clockID].append(activeThread);
    } else {
        timerThreads[clockID].insertBefore(it.get(), activeThread);
    }
    activeThread->setState(Thread::State::Timer(clockID, timestamp));

    throw DiscardStateAndSchedule(activeThread, move(sl), move(sl2));
}


size_t Scheduler::nextProcessorID{0};

void Scheduler::schedule(Thread *thread, bool online) noexcept {
    // TODO
    size_t processorID;
    if (thread->pinnedToProcessor != nullptr) {
        processorID = thread->pinnedToProcessor->id;
    } else {
        processorID = __atomic_fetch_add(&nextProcessorID, 1, __ATOMIC_RELAXED) % CurrentSystem.numProcessors;
    }
    Processors[processorID].scheduler.add(thread, online);
}

void Scheduler::removeOtherThread(Thread *thread) noexcept {
    assert(lock.isLocked());
    assert(thread != processor->activeThread());

    readyThreads[thread->currentPriority()].remove(thread);
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

void Scheduler::updateTimer() noexcept {
    uint64_t minTime = -1ull;
    for (size_t clockID = 0; clockID < ClockID::COUNT; clockID++) {
        if (!timerThreads[clockID].empty()) {
            optional<uint64_t> threadTime = CurrentSystem.time.nanoSecondsToSystemClock(
                        clockID, timerThreads[clockID].begin()->state().time());
            if (threadTime) {
                minTime = min(minTime, *threadTime);
            }
        }
    }
    if (minTime != -1ull) {
        ::setTimer(minTime);
    }
}

[[noreturn]]
void Scheduler::scheduleNext() noexcept {
    assert(!processor->activeThread());
    assert(lock.isLocked());
    assert(processor == CurrentProcessor());

    for (ssize_t prio = Thread::NUM_PRIORITIES - 1; prio >= 0; prio--) {
        if (!readyThreads[prio].empty()) {
            Thread *newActiveThread = readyThreads[prio].pop();
            processor->activeThread(newActiveThread);
            newActiveThread->setState(Thread::State::Active(processor));

            if (newActiveThread == idleThread) {
                cout << "activeThread on " << processor->id << " is our idle Thread" << endl;
                assert(readyThreads[prio].empty());
                assert(prio == 0);
            } else {
                cout << "activeThread on " << processor->id << " is now ";
                for (char c: newActiveThread->process->logName) {
                    cout << c;
                }
                cout <<": " << newActiveThread->handle() << " (" << newActiveThread << ")" << endl;
            }

            assert(newActiveThread->currentProcessor() == processor);
            updateTimer();
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
