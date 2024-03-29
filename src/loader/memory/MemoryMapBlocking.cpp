#include <memory/MemoryMapBlocking.hpp>
#include <common/utils.hpp>
#include <iostream>

namespace memoryMapBlocking {

class BlockedRegionsMap {
private:
    static constexpr size_t MAX_BLOCKED_REGIONS = 100;
    Region blockedRegions[MAX_BLOCKED_REGIONS];
    size_t numBlockedRegions = 0;
    size_t firmwareIndex = 0;
    size_t splitIndex = 0;
    size_t getIndex = 0;

public:
    void block(Region region);
    optional<Region> getNextAvailable(bool reset, size_t minimumSize);
    optional<Region> getNextBlocked(bool reset);
};

static BlockedRegionsMap allBlockedRegionsMap;
static BlockedRegionsMap reclaimableRegionsMap;
static size_t numFirmwareRegions = 0;
static GetRegionCallback callback;

optional<Region> BlockedRegionsMap::getNextAvailable(bool reset, size_t minimumSize) {
    /* if a Region cotains a blocked Region, we may have to return both the part before and after,
     * thus returning more regions than multiboot has. This variable remembers at which part we
     * are. */

    if (reset) {
        firmwareIndex = 0;
        splitIndex = 0;
    }

    while (firmwareIndex < numFirmwareRegions) {
        optional<Region> entry = callback(firmwareIndex);

        /* firmware may contain entries that are not usable memory */
        if (entry) {
            Region entryRegion = *entry;
            Region resultRegion = entryRegion;

            size_t splitsFound = 0;

            for (size_t blockedIndex = 0; blockedIndex < numBlockedRegions; blockedIndex++) {
                Region &blockedRegion = blockedRegions[blockedIndex];

                if (entryRegion.contains(blockedRegion)) {
                    splitsFound++;

                    if (splitsFound == splitIndex) {
                        size_t diff = blockedRegion.end() - resultRegion.start;
                        resultRegion.start = blockedRegion.end();
                        resultRegion.length -= diff;
                    }

                    if (splitsFound == splitIndex + 1) {
                        assert(resultRegion.end() > blockedRegion.start);
                        resultRegion.length -= resultRegion.end() - blockedRegion.start;
                    }
                } else if (entryRegion.overlaps(blockedRegion)) {
                    /* region not fully contians loader region, but overlaps one end. In theory this
                     * should not happen because GRUB should use usable memory to load our loader. It
                     * may however, in theory, use parts of memory at the end where not the whole page
                     * is usable. */
                    assert(!blockedRegion.contains(entryRegion));
                    assert(blockedRegion.start >= entryRegion.start);

                    cout << "Warning: blocked region reaches beyond usable region. blocked end "
                         << blockedRegion.end() << " usable end " << entryRegion.end() << endl;

                    assert(blockedRegion.end() - entryRegion.end() < PAGE_SIZE);

                    size_t diff = blockedRegion.start;
                    resultRegion.length -= diff;
                }
            }

            if (splitsFound < splitIndex) {
                /* all parts done for this entry, continue with next one */
                firmwareIndex++;
                splitIndex = 0;
            } else {
                splitIndex++;

                if (resultRegion.length >= minimumSize) {
                    return resultRegion;
                }
            }
        } else {
            /* no usable region in this entry */
            firmwareIndex++;
            assert(splitIndex == 0);
        }
    }
    return {};
}

void BlockedRegionsMap::block(Region region) {
    assert(numBlockedRegions < MAX_BLOCKED_REGIONS);

    alignedGrow(region.start, region.length, PAGE_SIZE);
    //cout << "Blocking region " << region.start << "," << region.length << endl;

    size_t insertIndex;
    for (insertIndex = 0; insertIndex < numBlockedRegions; insertIndex++) {
        if (blockedRegions[insertIndex].start >= region.start) {
            break;
        }
    }
    memmove(blockedRegions + insertIndex + 1,
            blockedRegions + insertIndex,
            (numBlockedRegions - insertIndex) * sizeof(Region));
    blockedRegions[insertIndex] = region;
    numBlockedRegions++;

    /* merge overlapping areas */
    for (size_t index = 0; index < numBlockedRegions-1; index++) {
        if (blockedRegions[index].overlaps(blockedRegions[index + 1]) || blockedRegions[index].end() == blockedRegions[index + 1].start) {
            //cout << "merging blockedRegion " << blockedRegions[index].start << "," << blockedRegions[index].length << " with "
            //    << blockedRegions[index+1].start << "," << blockedRegions[index+1].length << endl;

            blockedRegions[index].merge(blockedRegions[index + 1]);

            memmove(blockedRegions + index + 1,
                    blockedRegions + index + 2,
                    (numBlockedRegions - index - 2) * sizeof(Region));

            numBlockedRegions--;
        }
    }
}

optional<Region> BlockedRegionsMap::getNextBlocked(bool reset) {
    if (reset) {
        getIndex = 0;
    } else {
        getIndex++;
    }
    if (numBlockedRegions > getIndex) {
        return blockedRegions[getIndex];
    } else {
        return {};
    }
}

void initialize(size_t _numFirmwareRegions, GetRegionCallback _callback) {
    numFirmwareRegions = _numFirmwareRegions;
    callback = _callback;
}

void blockRegion(Region region, bool reclaimable) {
    allBlockedRegionsMap.block(region);
    if (reclaimable) {
        reclaimableRegionsMap.block(region);
    }
}

uint8_t *allocateHandOver(size_t numPages) {
    auto result = allBlockedRegionsMap.getNextAvailable(true, numPages * PAGE_SIZE);
    /* avoid fully blocking the first 64k, from which the secondary processor entry page needs to
     * be taken */
    if (result && result->start < (1ul << 16)) {
        result->start += PAGE_SIZE;
        result->length -= PAGE_SIZE;
        if (result->length < numPages * PAGE_SIZE) {
            result = allBlockedRegionsMap.getNextAvailable(false, numPages * PAGE_SIZE);
        }
    }
    if (!result) {
        cout << "Unable to find memory region to contain Handover structure of " << numPages
             << " pages." << endl;
        Panic();
    }

    Region handOverRegion = *result;
    handOverRegion.length = numPages * PAGE_SIZE;
    blockRegion(handOverRegion, true);
    return reinterpret_cast<uint8_t *>(handOverRegion.start);
}

optional<Region> findNextAvailableRegion(bool reset, size_t minimumSize) {
    return allBlockedRegionsMap.getNextAvailable(reset, minimumSize);
}

optional<Region> nextReclaimableRegion(bool reset) {
    return reclaimableRegionsMap.getNextBlocked(reset);
}

}
