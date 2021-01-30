#pragma once

#include <zagtos/Messaging.hpp>
#include "Port.hpp"

class Command {
private:

    Command(Port &port);

public:
    Port &port;
    const size_t slotID;
    CommandHeader &header;
    CommandTable &table;
    zagtos::SharedMemory dataMemory;

    Command(Port &port, ATACommand cmd, size_t length, bool write);
    ~Command();
    Command(Command &other) = delete;
};

