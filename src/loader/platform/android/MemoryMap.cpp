#include <MemoryMap.hpp>
#include <iostream>

namespace memoryMap {

extern "C" char _header;
extern "C" char _end;

optional<Region> findNextAvailableRegion(bool , size_t ) {
    return {};
}

uint8_t *allocateHandOver(size_t) {
    return nullptr;
}

void freezeAndExitFirmware() {
    /* do nothing */
}


optional<Region> firstAvailableRegion() {
    return findNextAvailableRegion(true, PAGE_SIZE);
}

optional<Region> nextAvailableRegion() {
    return findNextAvailableRegion(false, PAGE_SIZE);
}


optional<Region> firstReclaimableRegion() {
    return {};
}

optional<Region> nextReclaimableRegion() {
    return {};
}

}
