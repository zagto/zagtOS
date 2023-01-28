#pragma once

#include <zagtos/SharedMemory.hpp>
#include "Port.hpp"
#include "MemoryArea.hpp"

class Command {
private:
    Command(Device &device);

public:
    Device &device;
    const size_t slotID;
    CommandHeader &header;
    CommandTable &table;
    MemoryArea *memoryArea;

    Command(ATACommand cmd,
            uint64_t startSector,
            size_t startPage,
            size_t numSectors,
            bool write,
            MemoryArea *memoryArea);
    ~Command();
    Command(Command &other) = delete;
};

