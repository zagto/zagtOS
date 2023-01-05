#pragma once
#include <queue>
#include <memory>
#include <mutex>
#include <processes/Message.hpp>
#include <processes/ThreadList.hpp>
#include <processes/Thread.hpp>

class Process;
class Thread;
class Event;

class EventQueue {
private:
    threadList::List<&Thread::ownerReceptor> waitingThreads;
    // TODO: a linked list might work better here
    queue<unique_ptr<Event>> events;

public:
    mutex lock;
    /* should be seen as const besides during handover */
    shared_ptr<Process> process;

    EventQueue(const shared_ptr<Process> process) noexcept;
    EventQueue(const hos_v1::EventQueue &handOver, const vector<shared_ptr<Thread>> &allThreads);
    EventQueue(EventQueue &) = delete;
    ~EventQueue();

    unique_ptr<Event> getEvent() noexcept;
    void wait(Thread *thread) noexcept;
    void addEvent(unique_ptr<Event> message);
    void cancelEventsByTag(size_t tag) noexcept;
};
