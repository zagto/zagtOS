#ifndef SPAWNPROCESS_HPP
#define SPAWNPROCESS_HPP

#include <common/inttypes.hpp>
#include <tasks/UUID.hpp>

class Task;

class SpawnProcess {
private:
    size_t address;
    size_t length;
    size_t priority;

    const uint32_t *canUse;
    size_t numCanUse;

    UUID messageType;
    size_t messageSize;
    size_t messageAddress;

    size_t result;
public:

    bool perform(Task &task);
};

#endif // SPAWNPROCESS_HPP
