#include <common/common.hpp>
#include <system/System.hpp>
#include <processes/EventQueue.hpp>
#include <processes/Thread.hpp>
#include <processes/Scheduler.hpp>
#include <processes/Process.hpp>


EventQueue::EventQueue(const shared_ptr<Process> process) noexcept :
    process{move(process)} {}

EventQueue::EventQueue(const hos_v1::EventQueue &handOver,
                       const vector<shared_ptr<Thread>> &allThreads) {
    if (handOver.numWaitingThreads > 0) {
        /* a message queue should not have both waiting threads and pending messages */
        assert(handOver.numEvents == 0);
        for (size_t index = 0; index < handOver.numWaitingThreads; index++) {
            waitingThreads.append(allThreads[handOver.waitingThreadIDs[index]].get());
        }
    }
    if (handOver.numEvents > 0) {
        for (size_t index = 0; index < handOver.numEvents; index++) {
            auto event = make_unique<Event>(handOver.events[index]);
            events.push_back(move(event));
        }
    }
}

EventQueue::~EventQueue() {
    if (!waitingThreads.empty() || !events.empty()) {
        cout << "TODO: deleting EventQueue that still has values in it" << endl;
        Panic();
    }
}

unique_ptr<Event> EventQueue::getEvent() noexcept {
    assert(lock.isLocked());

    /* events can be cancelled by replacing them with a nullptr */
    while (!events.empty() && !events.top()) {
        events.pop();
    }
    if (events.empty()) {
        return nullptr;
    }

    auto msg = move(events.top());
    events.pop();
    return move(msg);
}

void EventQueue::wait(Thread *thread) noexcept {
    assert(lock.isLocked());
    assert(thread->process == process);
    waitingThreads.append(thread);
}

void EventQueue::addEvent(unique_ptr<Event> event) {
    scoped_lock sl(lock);

    if (waitingThreads.empty()) {
        /* can't wake a Thread, queue message */
        events.push_back(move(event));
    } else {
        Thread *wokenThread = waitingThreads.begin().get();
        size_t infoAddress = wokenThread->state().eventInfoAddress();
        /* writeEventInfo may throw - do it before actually removing the Thread so it is not lost
         * in Transition state */
        event->writeEventInfo(wokenThread->process, infoAddress);
        wokenThread->setState(Thread::State::Transition());
        waitingThreads.pop();
        Scheduler::schedule(wokenThread, true);
    }
}

/* replace events in the queue by a nullptr to cancel them. This is used if user space deletes a
 * port, for whose tag there are still events pending. */
void callback(unique_ptr<Event> &event, void *data) noexcept {
    size_t *tag = static_cast<size_t *>(data);
    if (event && event->eventInfo.tag == *tag) {
        event = nullptr;
    }
}

void EventQueue::cancelEventsByTag(size_t tag) noexcept {
    scoped_lock sl(lock);
    events.runCallback(callback, &tag);
}
