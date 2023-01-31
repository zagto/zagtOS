#pragma once

#include <cstdint>
#include <zagtos/Messaging.hpp>

class MemoryArea;

struct LogicalCommand {
public:
    enum class Action {
        IDENTIFY, READ, WRITE
    };

    MemoryArea *memoryArea;
    const Action action;
    const uint32_t cookie;
    const uint64_t startSector;
    const uint64_t startPage;
    const uint64_t numSectors;
    zagtos::RemotePort responsePort;
};
