#pragma once

#include <zagtos/EventListener.hpp>
#include <unordered_set>
#include "Port.hpp"

class MemoryArea;

class Device : public zagtos::EventListener {
public:
    const uint64_t sectorSize;
    const uint64_t numSectors;
    Port &port;
    std::unordered_set<std::unique_ptr<MemoryArea>> memoryAreas;

    Device(uint64_t sectorSize, uint64_t numSectors, Port &port);
    void handleEvent(const zagtos::Event &event) final;
};
