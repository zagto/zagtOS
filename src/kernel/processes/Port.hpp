#ifndef PORT_HPP
#define PORT_HPP

#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <processes/Message.hpp>

class Thread;

class Tag {};

class Port {
    Thread &thread;
    mutex lock;
    bool threadWaits;
    vector<shared_ptr<Tag>> acceptedTags;
    queue<unique_ptr<Message>> messages;

public:
    Port(Thread &thread, vector<shared_ptr<Tag>> &acceptedTags);
    Port(Port &) = delete;
    //~Port();

    bool ownedBy(const Thread &threadToCheck) {
        return &thread == &threadToCheck;
    }
    unique_ptr<Message> getMessageOrMakeThreadWait();
};

#endif // PORT_HPP
