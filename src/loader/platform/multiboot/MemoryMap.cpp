#include <MemoryMap.hpp>
#include <Multiboot.hpp>

namespace memoryMap {

static Region handOverRegion;
static Region loaderRegionReclaimable;
static MemoryMapTag *mmap;

extern "C" char _multiboot;
extern "C" char _end;

uint8_t *allocateHandOver(size_t numPages) {
    mmap = MultibootInfo->getTag<MemoryMapTag>(0);
    assert(mmap != nullptr);

    for (size_t index = 0; index < mmap->numEntries(); index++) {
        MemoryMapTag::Entry *entry = mmap->getEntry(index);
        if (entry->usableRegion()->length <= numPages * PAGE_SIZE) {
            handOverRegion = {entry->usableRegion()->start, numPages * PAGE_SIZE};

            /* update entry, so this memory is not detected again as usable */
            entry->length -= entry->usableRegion()->end() - handOverRegion.end();
            entry->baseAddress = handOverRegion.end();

            return reinterpret_cast<uint8_t *>(handOverRegion.start);
        }
    }

    cout << "Unable to find memory region to contain Handover structure of " << numPages
         << " pages." << endl;
    Panic();
}

void freezeAndExitFirmware() {
    /* do nothing */
}

static size_t currentIndex = 0;
static size_t foundLoaderHole = 0;

optional<Region> firstAvailableRegion() {
    currentIndex = 0;
    return nextAvailableRegion();
}

optional<Region> nextAvailableRegion() {
    Region loaderRegion(reinterpret_cast<size_t>(&_multiboot),
                        reinterpret_cast<size_t>(&_end) - reinterpret_cast<size_t>(&_multiboot));
    alignedGrow(loaderRegion.start, loaderRegion.length, PAGE_SIZE);

    while (currentIndex < mmap->numEntries()) {
        MemoryMapTag::Entry *entry = mmap->getEntry(currentIndex - foundLoaderHole);
        currentIndex++;

        if (entry->usableRegion()) {
            Region region = *entry->usableRegion();
            if (region.contains(loaderRegion)) {
                if (!foundLoaderHole) {
                    /* first part: return region before loader */
                    foundLoaderHole = true;
                    region.length = loaderRegion.start - region.start;
                } else {
                    /* second part: return region after loader */
                    region.length -= loaderRegion.end() - region.start;
                    region.start = loaderRegion.end();
                }
            } else if (region.overlaps(loaderRegion)) {
                /* region not fully contians loader region, but overlaps one end. In theory this
                 * should not happen because GRUB should use usable memory to load our loader. It
                 * may however, in theory, use parts of memory at the end where not the whole page
                 * is usable. */
                assert(!loaderRegion.contains(region));
                assert(loaderRegion.start >= region.start);

                region.length -= loaderRegion.end() - region.start;
                region.start = loaderRegion.end();
            }

            if (region.length > 0) {
                return region;
            }
        }

    }
    return {};
}

static bool returnedLoaderReclaimable = false;

optional<Region> firstReclaimableRegion() {
    returnedLoaderReclaimable = false;
    return handOverRegion;
}

optional<Region> nextReclaimableRegion() {
    if (!returnedLoaderReclaimable) {
        returnedLoaderReclaimable = true;
        return loaderRegionReclaimable;
    } else {
        return {};
    }
}

}
