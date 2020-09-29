#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <processes/Message.hpp>

class Process;
class Thread;

class Tag {};

class Port {
    mutex lock;
    Thread *waitingThread{nullptr};
    queue<unique_ptr<Message>> messages;

public:
    /* should be seen as const besides during handover */
    shared_ptr<Process> process;

    Port(const shared_ptr<Process> process);
    Port(const hos_v1::Port &handOver, const vector<shared_ptr<Thread>> &allThreads);
    Port(Port &) = delete;
    ~Port();

    unique_ptr<Message> getMessageOrMakeThreadWait(Thread *thread);
    void addMessage(unique_ptr<Message> message);
};
