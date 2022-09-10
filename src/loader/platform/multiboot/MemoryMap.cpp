#include <MemoryMap.hpp>
#include <Multiboot.hpp>
#include <iostream>

namespace memoryMap {

constexpr size_t MAX_BLOCKED_REGIONS = 5;
static Region blockedRegions[MAX_BLOCKED_REGIONS];
static size_t numBlockedRegions = 0;

static MemoryMapTag *mmap;

extern "C" char _multiboot;
extern "C" char _end;

optional<Region> findNextAvailableRegion(bool reset, size_t minimumSize) {
    assert(minimumSize % PAGE_SIZE == 0);
    assert(minimumSize > 0);

    static size_t multibootIndex = 0;
    /* if a Region cotains a blocked Region, we may have to return both the part before and after,
     * thus returning more regions than multiboot has. This variable remembers at which part we
     * are. */
    static size_t splitIndex = 0;

    if (reset) {
        multibootIndex = 0;
        splitIndex = 0;
    }

    while (multibootIndex < mmap->numEntries()) {
        MemoryMapTag::Entry *entry = mmap->getEntry(multibootIndex);

        if (entry->usableRegion()) {
            Region entryRegion = *entry->usableRegion();
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
                multibootIndex++;
                splitIndex = 0;
            } else {
                splitIndex++;

                if (resultRegion.length >= minimumSize) {
                    return resultRegion;
                }
            }
        } else {
            /* no usable region in this entry */
            multibootIndex++;
            assert(splitIndex == 0);
        }
    }
    return {};
}

static void blockRegion(Region region) {
    assert(numBlockedRegions < MAX_BLOCKED_REGIONS);

    cout << "Blocking region " << region.start << "," << region.length << endl;

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
            cout << "merging blockedRegion " << blockedRegions[index].start << "," << blockedRegions[index].length << " with "
                << blockedRegions[index+1].start << "," << blockedRegions[index+1].length << endl;

            blockedRegions[index].merge(blockedRegions[index + 1]);

            memmove(blockedRegions + index + 1,
                    blockedRegions + index + 2,
                    (numBlockedRegions - index - 2) * sizeof(Region));

            numBlockedRegions--;
        }
    }
}

static void initialize() {
    mmap = MultibootInfo->getTag<MemoryMapTag>(0);
    assert(mmap != nullptr);

    Region loaderRegion(reinterpret_cast<size_t>(&_multiboot),
                        reinterpret_cast<size_t>(&_end) - reinterpret_cast<size_t>(&_multiboot));
    alignedGrow(loaderRegion.start, loaderRegion.length, PAGE_SIZE);
    blockRegion(loaderRegion);

    Region multibootInfoRegion(reinterpret_cast<size_t>(MultibootInfo), MultibootInfo->totalSize);
    alignedGrow(multibootInfoRegion.start, multibootInfoRegion.length, PAGE_SIZE);
    blockRegion(multibootInfoRegion);


    size_t index = 0;
    while (true) {
        ModuleTag *mod = MultibootInfo->getTag<ModuleTag>(index);
        if (!mod) {
            return;
        }

        Region moduleRegion(mod->startAddress, mod->endAddress - mod->startAddress);
        alignedGrow(moduleRegion.start, moduleRegion.length, PAGE_SIZE);
        blockRegion(moduleRegion);

        index++;
    }
}

uint8_t *allocateHandOver(size_t numPages) {
    /* This is the first MemoryMap fuction called by the loader, since it has to modify the Memory
     * Map, therefore do initializations here */
    initialize();

    auto result = findNextAvailableRegion(true, numPages * PAGE_SIZE);
    /* avoid fully blocking the first 64k, from which the secondary processor entry page needs to
     * be taken */
    if (result && result->start < (1ul << 16)) {
        result->start += PAGE_SIZE;
        result->length -= PAGE_SIZE;
        if (result->length < numPages * PAGE_SIZE) {
            result = findNextAvailableRegion(false, numPages * PAGE_SIZE);
        }
    }
    if (!result) {
        cout << "Unable to find memory region to contain Handover structure of " << numPages
             << " pages." << endl;
        Panic();
    }

    Region handOverRegion = *result;
    handOverRegion.length = numPages * PAGE_SIZE;
    blockRegion(handOverRegion);
    return reinterpret_cast<uint8_t *>(handOverRegion.start);
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

static size_t numBlockedReturned = 0;

optional<Region> firstReclaimableRegion() {
    numBlockedReturned = 0;
    if (numBlockedRegions > 0) {
        return blockedRegions[numBlockedReturned];
    } else {
        return {};
    }
}

optional<Region> nextReclaimableRegion() {
    numBlockedReturned++;
    if (numBlockedRegions > numBlockedReturned) {
        return blockedRegions[numBlockedReturned];
    } else {
        return {};
    }
}

}
