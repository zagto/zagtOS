#include <MemoryMap.hpp>
#include <memory/MemoryMapBlocking.hpp>
#include <DeviceTree.hpp>
#include <iostream>

namespace memoryMap {

constexpr size_t MAX_NUM_ENTRIES = 100;
static size_t numEntries = 0;
static Region entries[MAX_NUM_ENTRIES];

extern "C" char _start;
extern "C" char _end;

optional<Region> callback(size_t firmwareIndex) {
    return entries[firmwareIndex];
}

optional<Region> findNextAvailableRegion(bool reset, size_t minimumSize) {
    return memoryMapBlocking::findNextAvailableRegion(reset,
                                                      minimumSize);
}

static void parseDeviceTree(const deviceTree::Tree &tree) {
    auto memoryNode = tree.rootNode.findChildNode("memory");
    if (!memoryNode) {
        cout << "could not find memory node" << endl;
        Panic();
    }
    numEntries = memoryNode->getNumRegions();
    for (size_t index = 0; index < numEntries; index++) {
        entries[index] = memoryNode->getRegionProperty(index);
    }
}

static void setupBlockedRegions(const deviceTree::Tree &tree) {
    memoryMapBlocking::initialize(numEntries, callback);

    Region loaderRegion(reinterpret_cast<size_t>(&_start),
                        reinterpret_cast<size_t>(&_end) - reinterpret_cast<size_t>(&_start));
    memoryMapBlocking::blockRegion(loaderRegion, true);

    Region dtbRegion(tree.memoryRegion());
    memoryMapBlocking::blockRegion(dtbRegion, true);

    /* reserved-memory node */
    auto reservedMemoryNode = tree.rootNode.findChildNode("reserved-memory");
    if (!reservedMemoryNode) {
        cout << "could not find reserved-memory node" << endl;
        Panic();
    }
    auto regionNode = reservedMemoryNode->findChildNode();
    while (regionNode) {
        size_t numRegions = regionNode->getNumRegions();
        for (size_t index = 0; index < numRegions; index++) {
            memoryMapBlocking::blockRegion(regionNode->getRegionProperty(index), false);
        }
        regionNode = reservedMemoryNode->findChildNode({}, regionNode->findFollowingToken());
    }

    /* DTB memory reservation block */
    size_t index = 0;
    Region region = tree.reservationBlockEntry(0);
    while (region != Region(0, 0)) {
        memoryMapBlocking::blockRegion(region, false);
        index++;
        region = tree.reservationBlockEntry(index);
    }
}

void initialize() {
    deviceTree::Tree tree;
    parseDeviceTree(tree);
    setupBlockedRegions(tree);
}

uint8_t *allocateHandOver(size_t numPages) {
    return memoryMapBlocking::allocateHandOver(numPages);
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
    return memoryMapBlocking::nextReclaimableRegion(true);
}

optional<Region> nextReclaimableRegion() {
    return memoryMapBlocking::nextReclaimableRegion(false);
}

}
