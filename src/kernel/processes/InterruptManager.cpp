#include <processes/InterruptManager.hpp>
#include <processes/Process.hpp>
#include <system/Processor.hpp>
#include <processes/Scheduler.hpp>
#include <algorithm>

BoundInterrupt::BoundInterrupt(InterruptType type,
                               size_t typeData,
                               TriggerMode triggerMode) :
        type{type},
        typeData{typeData},
        triggerMode{triggerMode} {

    /* don't lock ourselves here - nothing is really happening with the data at this point and
     * our lock before InterruptManager would violate the lock order. */

    /* assigns the ProcessorInterrupt */
    InterruptManager.bind(this);

    /* sets up interrupt routing in the hardware */
    try {
        CurrentSystem.bindInterrupt(*this);
    } catch (...) {
        /* we don't want to leak a bound InterruptManager entry */
        InterruptManager.unbind(this);
        throw;
    }
}

BoundInterrupt::~BoundInterrupt() noexcept {
    InterruptManager.unbind(this);
    CurrentSystem.unbindInterrupt(*this);

    scoped_lock sl(lock);

    while (subscriptions.size() != 0) {
        unsubscribe(subscriptions.begin() + (subscriptions.size() - 1));
    }
    assert(processingSubscribers == 0);
}

vector<BoundInterrupt::Subscription>::Iterator
BoundInterrupt::findSubscription(shared_ptr<Process> process) {
    return find_if(subscriptions.begin(),
                   subscriptions.end(),
                   [&process](Subscription &sub) {

                return sub.eventQueue->process == process;
        });
}

void BoundInterrupt::subscribe(shared_ptr<EventQueue> eventQueue, size_t eventTag) {
    scoped_lock sl(lock);

    assert(eventQueue->process == CurrentProcess());

    auto findResult = findSubscription(eventQueue->process);
    if (findResult != subscriptions.end()) {
        cout << "BoundInterrupt::subscribe: Process is already subscribed" << endl;
        throw BadUserSpace(eventQueue->process);
    }

    subscriptions.push_back({move(eventQueue), eventTag, startedOccurence, startedOccurence});
}

void BoundInterrupt::unsubscribe(vector<Subscription>::Iterator subscription) noexcept {
    assert(lock.isLocked());

    assert(subscription->processedOccurence <= startedOccurence);
    assert(processingSubscribers >= startedOccurence - subscription->processedOccurence);
    processingSubscribers -= startedOccurence - subscription->processedOccurence;
    subscriptions.erase(subscription);

    /* the unsubscribed Process might be the last one not having fully processed */
    checkFullyProcessed();
}

void BoundInterrupt::unsubscribe() {
    scoped_lock sl2(lock);

    auto subscription = findSubscription(CurrentProcess());
    if (subscription == subscriptions.end()) {
        cout << "Unsubscribe from interrupt that was not subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    unsubscribe(subscription);
}


void BoundInterrupt::processed() {
    scoped_lock sl(lock);

    auto process = CurrentProcess();
    auto subscription = findSubscription(process);
    if (subscription == subscriptions.end()) {
        cout << "Thread sent interrupt processed but was not subscribed" << endl;
        throw BadUserSpace(CurrentProcess());
    }

    if (subscription->processedOccurence == completedOccurence) {
        cout << "Thread sent interrupt processed for occurence it already processed" << endl;
        throw BadUserSpace(CurrentProcess());
    }
    assert(subscription->processedOccurence < completedOccurence);
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

void BoundInterrupt::occur() {
    scoped_lock sl(lock);

    /* this method may be retried multiple times after exceptions. make sure to not deliver
     * an event for an occurence we already delivered */
    if (startedOccurence == completedOccurence) {
        /* each subscriber now has to process once more */
        processingSubscribers += subscriptions.size();
        startedOccurence++;
    }

    for (Subscription& sub: subscriptions) {
        /* this method may be retried multiple times after exceptions. make sure to not deliver
         * an event for an occurence we already delivered */
        if (sub.deliveredOccurence < startedOccurence) {
            assert(sub.deliveredOccurence == startedOccurence - 1);
            userApi::ZoEventInfo eventInfo = {
                userApi::ZAGTOS_EVENT_INTERRUPT,
                sub.eventTag,
                {},
            };
            sub.eventQueue->addEvent(make_unique<Event>(eventInfo));
            sub.deliveredOccurence = startedOccurence;
        }
    }

    completedOccurence++;
    assert(completedOccurence == startedOccurence);
}

const ProcessorInterrupt &BoundInterrupt::processorInterrupt() const noexcept {
    return _processorInterrupt;
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
                binding->_processorInterrupt = {processorID, vectorNumber};
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
           [binding->_processorInterrupt.processorID][binding->_processorInterrupt.vectorNumber - vectorOffset];
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
