#ifndef PORT_HPP
#define PORT_HPP

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
    weak_ptr<Thread> waitingThread{};
    queue<unique_ptr<Message>> messages;

public:
    const shared_ptr<Process> process;

    Port(const shared_ptr<Process> process);
    Port(Port &) = delete;

    unique_ptr<Message> getMessageOrMakeThreadWait(shared_ptr<Thread> thread);
    void addMessage(unique_ptr<Message> message);
};

#endif // PORT_HPP
