#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <lib/vector.hpp>
#include <processes/UUID.hpp>

class Port;

class Message {
private:
    Port *from;
    Port *to;
    UUID messageType;
    UserVirtualAddress dataAddress;

public:
    Message();
    size_t dataSize();
};

#endif // MESSAGE_HPP
