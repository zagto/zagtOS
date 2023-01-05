#pragma once

#include "PortListener.hpp"

class Device : public PortListener {
public:
    const uint64_t sectorSize;
    const uint64_t numSectors;
    const uint64_t portID;

    Device(uint64_t sectorSize, uint64_t numSectors, size_t portID);
    void handleMessage(const zagtos::Event &event) final;
};
