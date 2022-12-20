#include <MemoryMap.hpp>
#include <memory/MemoryMapBlocking.hpp>
#include <Multiboot.hpp>
#include <iostream>

namespace memoryMap {
static MemoryMapTag *mmap;

extern "C" char _multiboot;
extern "C" char _end;

optional<Region> callback(size_t firmwareIndex) {
    return mmap->getEntry(firmwareIndex)->usableRegion();
}

optional<Region> findNextAvailableRegion(bool reset, size_t minimumSize) {
    return memoryMapBlocking::findNextAvailableRegion(reset,
                                                      minimumSize);
}


void initialize() {
    mmap = MultibootInfo->getTag<MemoryMapTag>(0);
    assert(mmap != nullptr);

    memoryMapBlocking::initialize(mmap->numEntries(), callback);

    Region loaderRegion(reinterpret_cast<size_t>(&_multiboot),
                        reinterpret_cast<size_t>(&_end) - reinterpret_cast<size_t>(&_multiboot));
    memoryMapBlocking::blockRegion(loaderRegion, true);

    Region multibootInfoRegion(reinterpret_cast<size_t>(MultibootInfo), MultibootInfo->totalSize);
    memoryMapBlocking::blockRegion(multibootInfoRegion, true);

    /* block multiboot module regions, i.e. kernel & process image */
    size_t index = 0;
    while (true) {
        ModuleTag *mod = MultibootInfo->getTag<ModuleTag>(index);
        if (!mod) {
            return;
        }

        Region moduleRegion(mod->startAddress, mod->endAddress - mod->startAddress);
        memoryMapBlocking::blockRegion(moduleRegion, true);

        index++;
    }
}

uint8_t *allocateHandOver(size_t numPages) {
    return memoryMapBlocking::allocateHandOver(numPages);
}

void freezeAndExitFirmware(const hos_v1::SerialInfo &, const hos_v1::FramebufferInfo &) {
    /* do nothing */
}


optional<Region> firstAvailableRegion() {
    return findNextAvailableRegion(true, PAGE_SIZE);
}

optional<Region> nextAvailableRegion() {
    return findNextAvailableRegion(false, PAGE_SIZE);
}

optional<Region> firstReclaimableRegion() {
    return memoryMapBlocking::nextReclaimableRegion(true);
}

optional<Region> nextReclaimableRegion() {
    return memoryMapBlocking::nextReclaimableRegion(false);
}

}
