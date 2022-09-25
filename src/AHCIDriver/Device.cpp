#include "Device.hpp"

Device::Device(uint64_t sectorSize, uint64_t numSectors) :
    sectorSize{sectorSize},
    numSectors{numSectors},
    port(zagtos::DefaultEventQueue, reinterpret_cast<size_t>(this)) {}
