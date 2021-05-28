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
    mappedAreas(handOver.numMappedAreas, nullptr, status) {
    if (!status) {
        return;
    }

    for (size_t index = 0; index < handOver.numMappedAreas; index++) {
        Result result = make_raw<MappedArea>(allMemoryAreas[handOver.mappedAreas[index].memoryAreaID],
                                             handOver.mappedAreas[index]);
        if (!result) {
            cout << "TODO: failure during handover" << endl;
            Panic();
        }
        mappedAreas[index] = *result;
        assert(mappedAreas[index]->region.isPageAligned());
        if (index > 0) {
            assert(mappedAreas[index]->region.start >= mappedAreas[index - 1]->region.end());
        }
    }
}

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
    for (MappedArea *&mappedArea: mappedAreas) {
        /* on handover constructor failure mappedAreas may contiain nullptrs */
        if (mappedArea != nullptr) {
            delete mappedArea;
            mappedArea = nullptr;
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
    assert(lock.isLocked());

    if (!UserVirtualAddress::checkInRegion(userAddress)) {
        return {};
    }

    auto [index, mapped] = findIndexFor(userAddress);
    if (!mapped) {
        return {};
    }

    MappedArea *mappedArea = mappedAreas[index];
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

Status ProcessAddressSpace::addAnonymous(Region region, Permissions permissions, bool overwriteExisiting) {
    scoped_lock sl(lock);

    if (!UserSpaceRegion.contains(region)) {
        cout << "Attempt to add region to ProcessAddressSpace that overlaps is not in "
             << "UserSpaceRegion" << endl;
        return Status::BadUserSpace();
    }

    auto [index, startAlreadyMapped] = findIndexFor(region.start);
    if (startAlreadyMapped
            || (index < mappedAreas.size() && mappedAreas[index]->region.overlaps(region))) {

        if (overwriteExisiting) {
            Status status = _removeRegion(region);
            if (!status) {
                return status;
            }
        } else {
            cout << "Attempt to add region to ProcessAddressSpace that overlaps with existing" << endl;
            return Status::BadUserSpace();
        }
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

    auto [region, index] = findFreeRegion(length);

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

Status ProcessAddressSpace::ensureSplitAt(size_t address) {
    auto [index, inUse] = findIndexFor(address);

    if (inUse && mappedAreas[index]->region.start != address) {
        /* area needs to be split */

        /* resize the vector first, since it would be hard to recover from an exeption later */
        Status status = mappedAreas.insert(nullptr, index + 1);
        if (!status) {
            return status;
        }

        auto [first, second] = mappedAreas[index]->split();
        mappedAreas[index] = first;
        mappedAreas[index + 1] = second;
    }
}

Status ProcessAddressSpace::ensureRegionIndependent(Region region) {
    ensureSplitAt(region.start);
    ensureSplitAt(region.end());
}


Status ProcessAddressSpace::removeRegion(Region region) {
    scoped_lock sl(lock);


    optional result = findMemoryArea(region.start, false);
    if (!result) {
        size_t index = mappedAreas
    }
}

/* This function may only fail if given an invalid startAddress. */
Status ProcessAddressSpace::removeMapping(size_t startAddress) {
    scoped_lock sl(lock);

    auto [index, mapped] = findIndexFor(startAddress);
    if (!mapped) {
        return Status::BadUserSpace();
    }
    mappedAreas.remove(index);
}

Status ProcessAddressSpace::handlePageFault(size_t address, Permissions requiredPermissions) {

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
