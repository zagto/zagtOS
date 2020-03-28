#ifndef SPAWNPROCESS_HPP
#define SPAWNPROCESS_HPP

#include <common/inttypes.hpp>
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
    size_t numMessageHandles;

    size_t result;
public:

    bool perform(Process &process);
};

#endif // SPAWNPROCESS_HPP
