#pragma once

#include <zagtos/Messaging.hpp>

class Device {
public:
    const uint64_t sectorSize;
    const uint64_t numSectors;
    const zagtos::Port port;

    Device(uint64_t sectorSize, uint64_t numSectors);
    Device(const Device &) = delete;
};
