#include <processes/InterruptManager.hpp>
#include <processes/Process.hpp>
#include <system/Processor.hpp>
#include <processes/Scheduler.hpp>
#include <syscalls/WaitInterrupt.hpp>
#include <algorithm>

BoundInterrupt::BoundInterrupt(InterruptType type,
                               size_t typeData,
                               TriggerMode triggerMode,
                               Polarity polarity) :
    type{type},
    typeData{typeData},
    triggerMode{triggerMode},
    polarity{polarity} {

    scoped_lock sl(KernelInterruptsLock);
    /* don't lock ourselves here - nothing is really happening with the data at this point and
     * our lock before InterruptManager would violate the lock order. */

    /* assigns the ProcessorInterrupt */
    InterruptManager.bind(this);

    /* sets up interrupt routing in the hardware */
    try {
        CurrentSystem.bindInterrutpt(*this);
    } catch (...) {
        /* we don't want to leak a bound InterruptManager entry */
        InterruptManager.unbind(this);
        throw;
    }
}

BoundInterrupt::~BoundInterrupt() noexcept {
    scoped_lock sl(KernelInterruptsLock);
    InterruptManager.unbind(this);
    CurrentSystem.unbindInterrupt(*this);

    scoped_lock sl2(lock);

    while (subscriptions.size() != 0) {
        unsubscribe(subscriptions.begin() + (subscriptions.size() - 1));
    }
    assert(processingSubscribers == 0);
    assert(waitingThreads.empty());

}

vector<BoundInterrupt::Subscription>::Iterator
BoundInterrupt::findSubscription(shared_ptr<Process> process) {
    return find_if(subscriptions.begin(),
                   subscriptions.end(),
                   [&process](Subscription &sub) {

                return sub.process == process;
        });
}

void BoundInterrupt::subscribe() {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    shared_ptr<Process> process = CurrentProcess();

    auto findResult = findSubscription(process);
    if (findResult != subscriptions.end()) {
        cout << "BoundInterrupt::subscribe: Process is already subscribed" << endl;
        throw BadUserSpace(process);
    }

    subscriptions.push_back({process, occurence});
}

void BoundInterrupt::unsubscribe(vector<Subscription>::Iterator subscription) noexcept {
    assert(lock.isLocked());

    assert(subscription->processedOccurence <= occurence);
    assert(processingSubscribers >= occurence - subscription->processedOccurence);
    processingSubscribers -= occurence - subscription->processedOccurence;

    /* If another thread from the current Process is currently, waiting, cacncel that */
    for (auto thread: waitingThreads) {
        if (thread->process == subscription->process) {
            waitingThreads.remove(thread);

            assert(thread->state().kind() == Thread::INTERRUPT);
            thread->setState(Thread::State::Transition());
            thread->kernelStack->userRegisterState()->setSyscallResult(WAIT_INTERRUPT_CANCELED);
            Scheduler::schedule(thread, true);
            break;
        }
    }

    subscriptions.erase(subscription);

    /* the unsubscribed Process might be the last one not having fully processed */
    checkFullyProcessed();
}

void BoundInterrupt::unsubscribe() {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    auto subscription = findSubscription(CurrentProcess());
    if (subscription == subscriptions.end()) {
        cout << "Unsubscribe from interrupt that was not subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    unsubscribe(subscription);
}


void BoundInterrupt::processed() {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    auto process = CurrentProcess();
    auto subscription = findSubscription(process);
    if (subscription == subscriptions.end()) {
        cout << "Thread sent interrupt processed but was not subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    if (subscription->processedOccurence == occurence) {
        cout << "Thread sent interrupt processed for occurence it already processed" << endl;
        throw BadUserSpace(CurrentProcess());
    }
    assert(subscription->processedOccurence < occurence);
    subscription->processedOccurence++;
    assert(processingSubscribers > 0);
    processingSubscribers--;

    checkFullyProcessed();
}

void BoundInterrupt::checkFullyProcessed() noexcept {
    if (processingSubscribers == 0) {
        CurrentSystem.interruptFullyProcessed(*this);
    }
}

void BoundInterrupt::wait() {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    auto process = CurrentProcess();
    auto subscription = findSubscription(process);
    if (subscription == subscriptions.end()) {
        cout << "Thread sent interrupt processed but was not subscribed" << endl;
        throw BadUserSpace(process);
    }

    if (subscription->processedOccurence < occurence) {
        /* there is already a new occurence not processed by this Thread */
        return;
    }

    /* the Thread should have processed all occurences so far - make it wait */
    assert(subscription->processedOccurence == occurence);

    Thread *thread = CurrentThread();
    waitingThreads.append(thread);

    /* Asymmetric lock/unlock: These two locks will not be unlocked once this
     * method leaves, but once the thread state is discarded. */
    Scheduler &scheduler = CurrentProcessor()->scheduler;
    scoped_lock sl3(scheduler.lock);

    scheduler.removeActiveThread();
    thread->setState(Thread::State::Interrupt(this));

    throw DiscardStateAndSchedule(thread, move(sl), move(sl3));
}

void BoundInterrupt::occur() noexcept {
    scoped_lock sl(KernelInterruptsLock);
    scoped_lock sl2(lock);

    /* each subscriber now has to process once more */
    processingSubscribers += subscriptions.size();

    occurence++;

    while (!waitingThreads.empty()) {
        Thread *thread = waitingThreads.pop();

        assert(thread->state().kind() == Thread::INTERRUPT);
        thread->setState(Thread::State::Transition());
        thread->kernelStack->userRegisterState()->setSyscallResult(WAIT_INTERRUPT_SUCCESS);
        Scheduler::schedule(thread, true);
    }
}


namespace interruptManager {

Manager::Manager() {
    size_t numProcessors = _HandOverSystem->numProcessors;
    size_t vectorsPerProcessor = DynamicInterruptRegion.length;

    allInterrupts.resize(numProcessors);

    for (size_t processorID = 0; processorID < numProcessors; processorID++) {
        allInterrupts[processorID] = vector<BoundInterrupt *>(vectorsPerProcessor, nullptr);
    }
}

void Manager::bind(BoundInterrupt *binding) {
    scoped_lock sl(lock);

    for (size_t processorID = 0; processorID < allInterrupts.size(); processorID++) {
        for (size_t vectorNumber = DynamicInterruptRegion.start;
             vectorNumber < DynamicInterruptRegion.end();
             vectorNumber++) {

            BoundInterrupt *&item = allInterrupts[processorID][vectorNumber - vectorOffset];
            if (item == nullptr) {
                /* found a free place */
                binding->processorInterrupt = {processorID, vectorNumber};
                item = binding;
                return;
            }
        }
    }

    cout << "TODO: what happens if we run out of interrupt vectors" << endl;
    Panic();
}

void Manager::unbind(BoundInterrupt *binding) noexcept {
    scoped_lock sl(lock);

    BoundInterrupt *&bound = allInterrupts
           [binding->processorInterrupt.processorID][binding->processorInterrupt.vectorNumber - vectorOffset];
    assert(bound != nullptr);
    assert(bound == binding);

    bound = nullptr;
}

void Manager::occur(ProcessorInterrupt processorInterrupt) noexcept {
    BoundInterrupt *boundInterrupt;

    scoped_lock sl(lock);
    boundInterrupt = allInterrupts
           [processorInterrupt.processorID][processorInterrupt.vectorNumber - vectorOffset];
    if (boundInterrupt == nullptr) {
        cout << "Warning: non-bound interrupt Processor " << processorInterrupt.processorID
             << " vector " << processorInterrupt.vectorNumber << " occured" << endl;
        return;
    }

    boundInterrupt->occur();
}


}
