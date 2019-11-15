#ifndef PORT_HPP
#define PORT_HPP

#include <lib/vector.hpp>

class Message;
class Process;

class Port {
    Process &process;
    vector<uint32_t> acceptedTags;
    uint32_t _id;

public:
    Port(Process &process, vector<uint32_t> acceptedTags);
    Port(Port &) = delete;
    ~Port();

    uint32_t id();

    bool messagesPending();

};

#endif // PORT_HPP
