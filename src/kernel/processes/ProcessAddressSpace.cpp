#include <processes/ProcessAddressSpace.hpp>
#include <processes/MappedArea.hpp>
#include <memory/TLBContext.hpp>

ProcessAddressSpace::ProcessAddressSpace(Status &status):
    pagingContext(status),
    inTLBContextOfProcessor(CurrentSystem.numProcessors, TLB_CONTEXT_ID_NONE, status) {}

ProcessAddressSpace::ProcessAddressSpace(const hos_v1::Process &handOver,
                                         const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
                                         Status &status):
    pagingContext(status),
    inTLBContextOfProcessor(CurrentSystem.numProcessors, TLB_CONTEXT_ID_NONE, status),
    mappedAreas(allMemoryAreas, handOver, status) {}


ProcessAddressSpace::~ProcessAddressSpace() {
    for (TLBContextID tlbID: inTLBContextOfProcessor) {
        if (tlbID != TLB_CONTEXT_ID_NONE) {
            /* removes the pointer to our PagingContext from the TLBContext, which would be
             * dangling once this object is destructed. Doing this before all the unmapping also
             * avoids creation of invalidate requests for all the TLB contexts for this address
             * space */
            TLBContexts[tlbID].remove(&pagingContext);
        }
    }
}

pair<size_t, bool> ProcessAddressSpace::findIndexFor(size_t userAddress) {
    static_assert (UserSpaceRegion.start == 0);
    if (!UserVirtualAddress::checkInRegion(userAddress)) {
        return {mappedAreas.size(), false};
    }

    UserVirtualAddress address{userAddress};
    size_t low = 0;
    size_t high = mappedAreas.size() - 1;
    while (low < high) {
        size_t index = low + (high - low) / 2;
        if (address.isInRegion(mappedAreas[index]->region)) {
            return {index, true};
        }
        if (address.value() < mappedAreas[index]->region.start) {
            if (index == low) {
                return {index, false};
            }
            high = index - 1;
        } else {
            low = index + 1;
        }
    }
    assert(low == high);
    if (address.isInRegion(mappedAreas[low]->region)) {
        return {low, true};
    } else {
        if (address.value() < mappedAreas[low]->region.start) {
            return {low, false};
        } else {
            return {low + 1, false};
        }
    }
}

optional<pair<shared_ptr<MemoryArea> &, Region>>
ProcessAddressSpace::findMemoryArea(size_t userAddress, bool requireWritePermissions) {
    if (!UserVirtualAddress::checkInRegion(userAddress)) {
        return {};
    }

    optional mappedAreaOptional = mappedAreas.findMappedArea(UserVirtualAddress(userAddress));
    if (!mappedAreaOptional) {
        return {};
    }

    MappedArea *mappedArea = *mappedAreaOptional;
    if (requireWritePermissions) {
        if (mappedArea->permissions != Permissions::READ_WRITE
                && mappedArea->permissions != Permissions::READ_WRITE_EXECUTE) {
            return {};
        }
    }

    Region maxRegionInside{userAddress - mappedArea->region.start + mappedArea->offset,
                           mappedArea->region.end() - userAddress};
    return {{mappedArea->memoryArea, maxRegionInside}};
}

Status ProcessAddressSpace::addAnonymous(Region region, Permissions permissions) {
    scoped_lock sl(lock);

    size_t index;
    if (!mappedAreas.isRegionFree(region, index)) {
        cout << "Attempt to add region to ProcessAddressSpace that overlaps with existing" << endl;
        return Status::BadUserSpace();
    }

    Result memoryArea = make_shared<MemoryArea>(region.length);
    if (!memoryArea) {
        return memoryArea.status();
    }

    Result ma = make_raw<MappedArea>(region, *memoryArea, 0, permissions);
    if (!ma) {
        return ma.status();
    }

    return mappedAreas.insert(*ma, index);
}

Result<Region> ProcessAddressSpace::addAnonymous(size_t length, Permissions permissions) {
    scoped_lock sl(lock);

    bool valid;
    size_t index;
    Region region = mappedAreas.findFreeRegion(length, valid, index);
    if (!valid) {
        return Status::BadUserSpace();
    }

    Result memoryArea = make_shared<MemoryArea>(region.length);
    if (!memoryArea) {
        return memoryArea.status();
    }

    Result ma = make_raw<MappedArea>(region, *memoryArea, 0, permissions);
    if (!ma) {
        return ma.status();
    }

    Status status = mappedAreas.insert(*ma, index);
    if (!status) {
        return status;
    }
    return (*ma)->region;
}

Status ProcessAddressSpace::removeRegion(Region region) {
    scoped_lock sl(lock);

    auto [startIndex, splitStart] = findIndexFor(region.start);
    auto [startIndex, splitEnd] = findIndexFor(region.start);

    if (isInUse) {
;
    }

    optional result = findMemoryArea(region.start, false);
    if (!result) {
        size_t index = mappedAreas.for
    }
}

void removeMapping(size_t startAddress) {

}

Status ProcessAddressSpace::copyFrom(uint8_t *destination,
                                     size_t address,
                                     size_t length) {
    scoped_lock sl(lock);

    while (length > 0) {
        optional result = findMemoryArea(address, false);
        if (!result) {
            return Status::BadUserSpace();
        }

        auto [memoryArea, maxRegionInside] = *result;
        size_t accessLength = min(length, maxRegionInside.length);
        Status status = memoryArea->copyFrom(destination, maxRegionInside.start, accessLength);
        if (!status) {
            return status;
        }

        destination += accessLength;
        address += accessLength;
        length -= accessLength;
    }
    return Status::OK();
}

Status ProcessAddressSpace::copyTo(size_t address,
                                   const uint8_t *source,
                                   size_t length,
                                   bool requireWritePermissions) {
    scoped_lock sl(lock);

    while (length > 0) {
        optional result = findMemoryArea(address, requireWritePermissions);
        if (!result) {
            return Status::BadUserSpace();
        }

        auto [memoryArea, maxRegionInside] = *result;
        size_t accessLength = min(length, maxRegionInside.length);
        Status status = memoryArea->copyTo(maxRegionInside.start, source, accessLength);
        if (!status) {
            return status;
        }

        source += accessLength;
        address += accessLength;
        length -= accessLength;
    }
    return Status::OK();
}

Status ProcessAddressSpace::copyFromOhter(size_t destinationAddress,
                                          ProcessAddressSpace &sourceSpace,
                                          size_t sourceAddress,
                                          size_t length,
                                          bool requireWriteAccessToDestination) {
    scoped_lock sl(lock, sourceSpace.lock);

    while (length > 0) {
        optional sourceResult = findMemoryArea(sourceAddress, false);
        if (!sourceResult) {
            return Status::BadUserSpace();
        }
        auto [sourceArea, sourceMaxRegion] = *sourceResult;

        optional result = findMemoryArea(destinationAddress, requireWriteAccessToDestination);
        if (!result) {
            return Status::BadUserSpace();
        }
        auto [destinationArea, destinationMaxRegion] = *result;

        size_t accessLength = min(length,
                                  min(sourceMaxRegion.length, destinationMaxRegion.length));
        Status status = destinationArea->copyFromOther(destinationMaxRegion.start,
                                                       *sourceArea,
                                                       sourceMaxRegion.start,
                                                       accessLength);
        if (!status) {
            return status;
        }

        sourceAddress += accessLength;
        destinationAddress += accessLength;
        length -= accessLength;
    }
    return Status::OK();
}


Result<uint32_t> ProcessAddressSpace::atomicCopyFrom32(size_t address) {
    scoped_lock sl(lock);

    optional result = findMemoryArea(address, true);
    if (!result) {
        return Status::BadUserSpace();
    }

    auto [memoryArea, maxRegionInside] = *result;
    return memoryArea->atomicCopyFrom32(maxRegionInside.start);
}

/* PhysicalAddress returned for Futex manager use only. May have changed already since
 * ProcessAddressSpace is unlocked! */
Result<bool> ProcessAddressSpace::atomicCompareExchange32(size_t address,
                                                              uint32_t expectedValue,
                                                              uint32_t newValue) {
    scoped_lock sl(lock);

    optional result = findMemoryArea(address, true);
    if (!result) {
        return Status::BadUserSpace();
    }

    auto [memoryArea, maxRegionInside] = *result;
    return memoryArea->atomicCompareExchange32(maxRegionInside.start, expectedValue, newValue);
}
