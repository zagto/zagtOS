#pragma once

#include <vector>
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

    UserVirtualAddress headerAddress;
};
