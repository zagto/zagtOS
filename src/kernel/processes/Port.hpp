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
    Thread *waitingThread{nullptr};
    queue<unique_ptr<Message>> messages;

public:
    const shared_ptr<Process> process;

    Port(const shared_ptr<Process> process);
    Port(Port &) = delete;
    ~Port();

    unique_ptr<Message> getMessageOrMakeThreadWait(Thread *thread);
    void addMessage(unique_ptr<Message> message);
};

#endif // PORT_HPP
