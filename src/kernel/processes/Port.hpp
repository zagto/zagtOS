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
    Process &process;
    mutex lock;
    Thread *waitingThread{nullptr};
    queue<unique_ptr<Message>> messages;

public:
    Port(Process &process);
    Port(Port &) = delete;
    //~Port();

    unique_ptr<Message> getMessageOrMakeThreadWait(Thread *thread);
};

#endif // PORT_HPP
