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

    const UUID *canUse;
    size_t numCanUse;
    const UUID *askUse;
    size_t numAskUse;
    const UUID *canProvide;
    size_t numCanProvide;

    UUID messageType;
    size_t messageSize;
    size_t messageAddress;

    size_t result;
public:

    bool perform(Task &task);
};

#endif // SPAWNPROCESS_HPP
