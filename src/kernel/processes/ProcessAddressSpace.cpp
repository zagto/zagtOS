#include <processes/ProcessAddressSpace.hpp>
#include <processes/MappedArea.hpp>
#include <memory/TLBContext.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <system/Processor.hpp>

ProcessAddressSpace::ProcessAddressSpace(Status &status):
    pagingContext(status),
    inTLBContextOfProcessor(CurrentSystem.numProcessors, TLB_CONTEXT_ID_NONE, status) {}

ProcessAddressSpace::ProcessAddressSpace(const hos_v1::Process &handOver,
                                         const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
                                         Status &status):
    pagingContext(status),
    inTLBContextOfProcessor(CurrentSystem.numProcessors, TLB_CONTEXT_ID_NONE, status),
    mappedAreas(handOver.numMappedAreas, status) {
    if (!status) {
        return;
    }

    for (size_t index = 0; index < handOver.numMappedAreas; index++) {
        Result result = make_unique<MappedArea>(*this,
                                                allMemoryAreas[handOver.mappedAreas[index].memoryAreaID],
                                                handOver.mappedAreas[index]);
        if (!result) {
            cout << "TODO: failure during handover" << endl;
            Panic();
        }
        mappedAreas[index] = move(*result);
        assert(mappedAreas[index]->region.isPageAligned());
        if (index > 0) {
            assert(mappedAreas[index]->region.start >= mappedAreas[index - 1]->region.end());
        }
    }
}

ProcessAddressSpace::~ProcessAddressSpace() {
    for (TLBContextID &tlbID: inTLBContextOfProcessor) {
        if (tlbID != TLB_CONTEXT_ID_NONE) {
            /* removes the pointer to our PagingContext from the TLBContext, which would be
             * dangling once this object is destructed. Doing this before all the unmapping also
             * avoids creation of invalidate requests for all the TLB contexts for this address
             * space */
            TLBContexts[tlbID].remove(&pagingContext);
            tlbID = TLB_CONTEXT_ID_NONE;
        }
    }
}

bool ProcessAddressSpace::operator==(const ProcessAddressSpace &other) const {
    return this == &other;
}

pair<size_t, bool> ProcessAddressSpace::findIndexFor(size_t userAddress) {
    static_assert (UserSpaceRegion.start == 0);
    if (!UserVirtualAddress::checkInRegion(userAddress)) {
        return {mappedAreas.size(), false};
    }

    if (mappedAreas.size() == 0) {
        return {0, false};
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

optional<pair<shared_ptr<MemoryArea>, Region>>
ProcessAddressSpace::findMemoryArea(size_t userAddress, bool requireWritePermissions) {
    assert(lock.isLocked());

    if (!UserVirtualAddress::checkInRegion(userAddress)) {
        return {};
    }

    auto [index, mapped] = findIndexFor(userAddress);
    if (!mapped) {
        return {};
    }

    unique_ptr<MappedArea> &mappedArea = mappedAreas[index];
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

Result<pair<Region, size_t>> ProcessAddressSpace::findFreeRegion(size_t length) const {
    /* TODO: Address space layout randomization could happen here */
    assert(lock.isLocked());

    if (mappedAreas.size() == 0) {
        if (length < UserSpaceRegion.length - PAGE_SIZE) {
            return {{Region(PAGE_SIZE, length), 0}};
        } else {
            cout << "findFreeRegion: Process exhausted address space" << endl;
            return Status::BadUserSpace();
        }
    }

    if (mappedAreas[0]->region.start > PAGE_SIZE
            && length < mappedAreas[0]->region.start - PAGE_SIZE) {
        return {{Region(PAGE_SIZE, length), 0}};
    }

    for (size_t index = 1; index < mappedAreas.size(); index++) {
        if (mappedAreas[index]->region.start - mappedAreas[index - 1]->region.end() >= length) {
            return {{Region(mappedAreas[index - 1]->region.end(), length), index}};
        }
    }

    /* TODO: check end (normally stack is here */

    cout << "findFreeRegion: Process exhausted address space" << endl;
    return Status::BadUserSpace();
}

void ProcessAddressSpace::activate() {
    scoped_lock sl(tlbIDsLock);
    TLBContextID &tlbID = inTLBContextOfProcessor[CurrentProcessor->id];
    tlbID = CurrentProcessor->activatePagingContext(&pagingContext, tlbID);
}

Result<UserVirtualAddress> ProcessAddressSpace::add(Region region,
                                                    size_t offset,
                                                    shared_ptr<MemoryArea> memoryArea,
                                                    Permissions permissions,
                                                    bool overwriteExisiting) {
    scoped_lock sl(lock);

    if (!UserSpaceRegion.contains(region)) {
        cout << "Attempt to add region to ProcessAddressSpace that overlaps is not in "
             << "UserSpaceRegion" << endl;
        return Status::BadUserSpace();
    }

    assert(offset + region.length <= memoryArea->length);

    auto [index, startAlreadyMapped] = findIndexFor(region.start);
    if (region.start == 0
            || startAlreadyMapped
            || (index < mappedAreas.size() && mappedAreas[index]->region.overlaps(region))) {

        if (overwriteExisiting) {
            Status status = _removeRegion(region);
            if (!status) {
                return status;
            }
        } else {
            Result result = findFreeRegion(region.length);
            if (!result) {
                return result.status();
            }
            region = result->first;
            index = result->second;
        }
    }

    Result ma = make_unique<MappedArea>(*this, region, move(memoryArea), offset, permissions);
    if (!ma) {
        return ma.status();
    }

    Status status = mappedAreas.insert(move(*ma), index);
    if (status) {
        return UserVirtualAddress(region.start);
    } else {
        return status;
    }
}

Result<UserVirtualAddress> ProcessAddressSpace::addAnonymous(Region region,
                                                             Permissions permissions,
                                                             bool overwriteExisiting,
                                                             bool shared) {
    Result memoryArea = make_shared<MemoryArea>(shared,
                                                Permissions::READ_WRITE_EXECUTE,
                                                region.length);
    if (!memoryArea) {
        return memoryArea.status();
    }

    return add(region, 0, *memoryArea, permissions, overwriteExisiting);
}

Result<UserVirtualAddress> ProcessAddressSpace::addAnonymous(size_t length, Permissions permissions) {
    scoped_lock sl(lock);

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    Result result = findFreeRegion(length);
    if (!result) {
        return result.status();
    }
    auto [region, index] = *result;

    Result memoryArea = make_shared<MemoryArea>(false,
                                                Permissions::READ_WRITE_EXECUTE,
                                                region.length);
    if (!memoryArea) {
        return memoryArea.status();
    }

    Result ma = make_unique<MappedArea>(*this, region, *memoryArea, 0, permissions);
    if (!ma) {
        return ma.status();
    }

    Status status = mappedAreas.insert(move(*ma), index);
    if (!status) {
        return status;
    }
    return {region.start};
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

        Result result = mappedAreas[index]->split(address - mappedAreas[index]->region.start);
        if (!result) {
            /* erase the already reserved vector space for the second element */
            mappedAreas.erase(mappedAreas.begin() + index + 1);
            return result.status();
        }
        mappedAreas[index] = move(result->first);
        mappedAreas[index + 1] = move(result->second);
    }
    return Status::OK();
}

Status ProcessAddressSpace::ensureRegionIndependent(Region region) {
    Status status = ensureSplitAt(region.start);
    if (!status) {
        return status;
    }
    return ensureSplitAt(region.end());
}


Status ProcessAddressSpace::_removeRegion(Region region) {
    assert(lock.isLocked());

    Status status = ensureRegionIndependent(region);
    if (!status) {
        return status;
    }

    auto [startIndex, mapped] = findIndexFor(region.start);
    auto startIterator = mappedAreas.begin() + startIndex;
    auto endIterator = startIterator;
    while (endIterator != mappedAreas.end() && (*endIterator)->region.start < region.end()) {
        endIterator++;
    }

    mappedAreas.erase(startIterator, endIterator);
    return Status::OK();
}

Status ProcessAddressSpace::removeRegion(Region region) {
    scoped_lock sl(lock);
    return _removeRegion(region);
}

Result<size_t> ProcessAddressSpace::changeRegionProtection(Region region, Permissions permissions) {
    scoped_lock sl(lock);

    Status status = ensureRegionIndependent(region);
    if (!status) {
        return status;
    }

    auto [startIndex, mapped] = findIndexFor(region.start);
    auto startIterator = mappedAreas.begin() + startIndex;
    auto endIterator = startIterator;
    size_t previousEnd = region.start;

    while (endIterator != mappedAreas.end() && (*endIterator)->region.start < region.end()) {
        if ((*endIterator)->region.start != previousEnd) {
            cout << "changeRegionProtection called on a region with mapping holes" << endl;
            return ENOMEM;
        }
        if (!(*endIterator)->memoryArea->allowesPermissions(permissions)) {
            cout << "changeRegionProtection called on a region which does not allow the requested"
                << "permissions" << endl;
            return EACCESS;
        }
        previousEnd = (*endIterator)->region.end();
        endIterator++;
    }
    if (previousEnd != region.end()) {
        cout << "changeRegionProtection called on a region with mapping holes" << endl;
        return ENOMEM;
    }

    for (auto it = startIterator; it != endIterator; it++) {
        (*it)->permissions = permissions;
    }
    return 0;
}

/* This function may only fail if given an invalid startAddress. */
Status ProcessAddressSpace::removeMapping(size_t startAddress) {
    scoped_lock sl(lock);

    auto [index, mapped] = findIndexFor(startAddress);
    if (!mapped || mappedAreas[index]->region.start != startAddress) {
        cout << "removeMapping: give address " << startAddress << " is not the start of a mapping"
            << endl;
        return Status::BadUserSpace();
    }
    mappedAreas.erase(mappedAreas.begin() + index);
    return Status::OK();
}

Status ProcessAddressSpace::handlePageFault(size_t address, Permissions requiredPermissions) {
    scoped_lock sl(lock);

    auto [index, mapped] = findIndexFor(address);
    if (!mapped) {
        cout << "Process tried to access address " << address << " which it has not mapped "
            << endl;
        return Status::BadUserSpace();
    }

    unique_ptr<MappedArea> &mappedArea = mappedAreas[index];

    if (mappedArea->permissions == Permissions::INVALID) {
        cout << "Process tried to access address " << address
             << " which is explicitly access-disabled." << endl;
        return Status::BadUserSpace();
    }

    if (requiredPermissions > mappedArea->permissions) {
        cout << "Process tried to access address " << address << " using permissions "
            << requiredPermissions << " which is mapped with " << mappedArea->permissions << endl;
    }

    return mappedArea->ensurePagedIn(align(address, PAGE_SIZE, AlignDirection::DOWN));
}

Result<uint64_t> ProcessAddressSpace::getFutexID(size_t address) {
    scoped_lock sl(lock);

    optional result = findMemoryArea(address, true);
    if (!result) {
        return Status::BadUserSpace();
    }

    auto [memoryArea, maxRegionInside] = *result;
    return memoryArea->getFutexID(maxRegionInside.start);
}

Status ProcessAddressSpace::copyFromLocked(uint8_t *destination, size_t address, size_t length) {
    assert(lock.isLocked());

    while (length > 0) {
        optional result = findMemoryArea(address, false);
        if (!result) {
            cout << "copyFrom: could not find memory area for " << address << endl;
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

Status ProcessAddressSpace::copyFrom(uint8_t *destination, size_t address, size_t length) {
    scoped_lock sl(lock);
    return copyFromLocked(destination, address, length);
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
        optional sourceResult = sourceSpace.findMemoryArea(sourceAddress, false);
        if (!sourceResult) {
            cout << "copyFromUser: Unable to find corresponding MemoryArea in source" << endl;
            return Status::BadUserSpace();
        }
        auto [sourceArea, sourceMaxRegion] = *sourceResult;

        optional result = findMemoryArea(destinationAddress, requireWriteAccessToDestination);
        if (!result) {
            cout << "copyFromUser: Unable to find corresponding MemoryArea in destination" << endl;
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
