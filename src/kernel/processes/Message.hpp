#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <lib/vector.hpp>
#include <processes/UUID.hpp>

class Process;

class Message {
private:
    Process *from;
    Process *to;
    UUID messageType;
    vector<uint8_t> data;

public:
    Message();
    size_t dataSize();
};

#endif // MESSAGE_HPP
