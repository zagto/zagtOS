#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <lib/vector.hpp>
#include <tasks/UUID.hpp>

class Task;

class Message {
private:
    Task *from;
    Task *to;
    UUID messageType;
    vector<uint8_t> data;

public:
    Message();
    size_t dataSize();
};

#endif // MESSAGE_HPP
