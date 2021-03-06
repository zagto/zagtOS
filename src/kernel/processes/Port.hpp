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
private:
    Thread *waitingThread{nullptr};
    /* index into the array of handles the Thread got with the ReceiveMessage call */
    size_t waitingThreadIndex{0};
    queue<unique_ptr<Message>> messages;

public:
    mutex lock;
    /* should be seen as const besides during handover */
    shared_ptr<Process> process;

    Port(const shared_ptr<Process> process) noexcept;
    Port(const hos_v1::Port &handOver, const vector<shared_ptr<Thread>> &allThreads);
    Port(Port &) = delete;
    ~Port();

    unique_ptr<Message> getMessage();
    void setWaitingThread(Thread *thread, size_t index) noexcept;
    void addMessage(unique_ptr<Message> message);
};
