#include <processes/InterruptManager.hpp>
#include <processes/Process.hpp>
#include <system/Processor.hpp>
#include <processes/Scheduler.hpp>

void PlatformInterrupt::initialize(size_t processorID, size_t vectorNumber) noexcept {
    this->processorID = processorID;
    this->vectorNumber = vectorNumber;
}

void PlatformInterrupt::subscribe(ProcessInterrupt &interrupt) {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    if (interrupt.subscribed) {
        cout << "Tried to subscribe to interrupt that is already subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    interrupt.subscribed = true;
    interrupt.processedOccurence = occurence;
    totalSubscribers++;
}

void PlatformInterrupt::unsubscribe(ProcessInterrupt &interrupt) {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    if (!interrupt.subscribed) {
        cout << "Tried to unsubscribe Thread from interrupt that was not subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    totalSubscribers--;
    if (interrupt.processedOccurence == occurence - 1) {
        processingSubscribers--;
    } else {
        assert(interrupt.processedOccurence == occurence);
    }
    interrupt.subscribed = false;
    interrupt.processedOccurence = 0;
}

void PlatformInterrupt::processed(ProcessInterrupt &interrupt) {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    if (!interrupt.subscribed) {
        cout << "Thread sent interrupt processed but was not subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    if (interrupt.processedOccurence == occurence) {
        cout << "Thread sent interrupt processed for occurence it already processed" << endl;
        throw BadUserSpace(CurrentProcess());
    }
    assert(interrupt.processedOccurence == occurence - 1);
    interrupt.processedOccurence = occurence;
    processingSubscribers--;

    if (processingSubscribers == 0) {
        // something needs to happen here
    }
}

void PlatformInterrupt::wait(ProcessInterrupt &interrupt) {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    if (!interrupt.subscribed) {
        cout << "Thread tried to wait for interrupt but was not subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    if (interrupt.processedOccurence == occurence - 1) {
        /* there is already a new occurence not processed by this Thread */
        return;
    }

    /* the Thread should have processed all occurences so far - make it wait */
    assert(interrupt.processedOccurence == occurence);

    Thread *thread = CurrentThread();
    waitingThreads.append(thread);

    /* Asymmetric lock/unlock: These two locks will not be unlocked once this
     * method leaves, but once the thread state is discarded. */
    Scheduler &scheduler = CurrentProcessor()->scheduler;
    scoped_lock sl3(scheduler.lock);

    scheduler.removeActiveThread();
    thread->setState(Thread::State::Interrupt(processorID, vectorNumber));

    throw DiscardStateAndSchedule(thread, move(sl), move(sl3));
}

void PlatformInterrupt::occur() noexcept {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    if (processingSubscribers != 0) {
        cout << "Interrupt " << vectorNumber << " on Processor " << processorID
             << " occured before the last occurence was fully processed." << endl;
        Panic();
    }

    size_t numWoken = 0;
    while (!waitingThreads.empty()) {
        Thread *thread = waitingThreads.pop();

        assert(thread->state().kind() == Thread::INTERRUPT);
        thread->setState(Thread::State::Transition());

        Scheduler::schedule(thread, true);
        numWoken++;
    }

    processingSubscribers = totalSubscribers;
}

ProcessInterrupt::ProcessInterrupt() noexcept :
    platformInterrupt{InterruptManager.getAny()} {}

ProcessInterrupt::~ProcessInterrupt() {
    scoped_lock sl(platformInterrupt.lock);

    if (subscribed) {
        platformInterrupt.unsubscribe(*this);
    }
}

namespace interruptManager {

InterruptManagerClass::InterruptManagerClass() {
    size_t numProcessors = _HandOverSystem->numProcessors;
    size_t vectorsPerProcessor = DynamicInterruptRegion.length;

    platformInterrupts = move(vector<vector<PlatformInterrupt>>(numProcessors));

    for (size_t processorID = 0; processorID < numProcessors; processorID++) {
        platformInterrupts[processorID] = move(vector<PlatformInterrupt>(vectorsPerProcessor));

        for (size_t vectorNumber = DynamicInterruptRegion.start;
             vectorNumber < DynamicInterruptRegion.end();
             vectorNumber++) {

            platformInterrupts[processorID][vectorNumber - vectorOffset].initialize(
                        processorID, vectorNumber);
        }
    }
}

PlatformInterrupt &InterruptManagerClass::getAny() noexcept {
    return platformInterrupts[0][0]; // TODO
}

void InterruptManagerClass::occur(size_t processorID, size_t vectorNumber) noexcept {
    platformInterrupts[processorID][vectorNumber - vectorOffset].occur();
}


}
