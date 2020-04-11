#pragma once

#include <common/inttypes.hpp>
#include <memory>
#include <processes/UUID.hpp>

class Process;

class SpawnProcess {
private:
    size_t address;
    size_t length;
    size_t priority;

    UUID messageType;
    size_t messageAddress;
    size_t messageSize;
    uint32_t numMessageHandles;

    uint32_t result;
public:

    bool perform(const shared_ptr<Process> &process);
};
