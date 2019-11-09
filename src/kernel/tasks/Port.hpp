#ifndef PORT_HPP
#define PORT_HPP

#include <lib/vector.hpp>

class Message;

class Port {
    vector<uint32_t> acceptedTags;
    uint32_t _id;

public:
    Port(vector<uint32_t> acceptedTags);
    ~Port();

    uint32_t id();
};

#endif // PORT_HPP
