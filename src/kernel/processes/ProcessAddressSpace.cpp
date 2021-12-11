#include <processes/ProcessAddressSpace.hpp>
#include <processes/MappedArea.hpp>
#include <memory/TLBContext.hpp>
#include <syscalls/ErrorCodes.hpp>
#include <system/Processor.hpp>
#include <processes/Process.hpp>

ProcessAddressSpace::ProcessAddressSpace():
    inTLBContextOfProcessor(CurrentSystem.numProcessors, TLB_CONTEXT_ID_NONE) {}

ProcessAddressSpace::ProcessAddressSpace(const hos_v1::Process &handOver,
                                         const vector<shared_ptr<MemoryArea>> &allMemoryAreas):
    inTLBContextOfProcessor(CurrentSystem.numProcessors, TLB_CONTEXT_ID_NONE),
    mappedAreas(handOver.numMappedAreas) {

    for (size_t index = 0; index < handOver.numMappedAreas; index++) {
        mappedAreas[index] = make_unique<MappedArea>(*this,
                                                allMemoryAreas[handOver.mappedAreas[index].memoryAreaID],
                                                handOver.mappedAreas[index]);
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

bool ProcessAddressSpace::operator==(const ProcessAddressSpace &other) const noexcept {
    return this == &other;
}

pair<size_t, bool> ProcessAddressSpace::findIndexFor(size_t userAddress) noexcept {
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
ProcessAddressSpace::findMemoryArea(size_t userAddress, bool requireWritePermissions) noexcept {
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

pair<Region, size_t> ProcessAddressSpace::findFreeRegion(size_t length) const {
    /* TODO: Address space layout randomization could happen here */
    assert(lock.isLocked());

    if (mappedAreas.size() == 0) {
        if (length < UserSpaceRegion.length - PAGE_SIZE) {
            return {Region(PAGE_SIZE, length), 0};
        } else {
            cout << "findFreeRegion: Process exhausted address space" << endl;
            throw BadUserSpace(process());
        }
    }

    if (mappedAreas[0]->region.start > PAGE_SIZE
            && length < mappedAreas[0]->region.start - PAGE_SIZE) {
        return {Region(PAGE_SIZE, length), 0};
    }

    for (size_t index = 1; index < mappedAreas.size(); index++) {
        if (mappedAreas[index]->region.start - mappedAreas[index - 1]->region.end() >= length) {
            return {Region(mappedAreas[index - 1]->region.end(), length), index};
        }
    }

    /* TODO: check end (normally stack is here */

    cout << "findFreeRegion: Process exhausted address space" << endl;
    throw BadUserSpace(process());
}

void ProcessAddressSpace::activate() {
    scoped_lock sl(tlbIDsLock);

    Processor *processor = CurrentProcessor();
    TLBContextID &tlbID = inTLBContextOfProcessor[processor->id];
    tlbID = processor->activatePagingContext(&pagingContext, tlbID);
}

UserVirtualAddress ProcessAddressSpace::add(Region region,
                                            size_t offset,
                                            shared_ptr<MemoryArea> memoryArea,
                                            Permissions permissions,
                                            bool overwriteExisiting) {
    scoped_lock sl(lock);

    if (!UserSpaceRegion.contains(region)) {
        cout << "Attempt to add region to ProcessAddressSpace that overlaps is not in "
             << "UserSpaceRegion" << endl;
        throw BadUserSpace(process());
    }

    assert(offset + region.length <= memoryArea->length);

    auto [index, startAlreadyMapped] = findIndexFor(region.start);
    if (region.start == 0
            || startAlreadyMapped
            || (index < mappedAreas.size() && mappedAreas[index]->region.overlaps(region))) {

        if (overwriteExisiting) {
            _removeRegion(region);
        } else {
            auto result = findFreeRegion(region.length);
            region = result.first;
            index = result.second;
        }
    }

    auto ma = make_unique<MappedArea>(*this, region, move(memoryArea), offset, permissions);

    mappedAreas.insert(move(ma), index);
    return UserVirtualAddress(region.start);
}

UserVirtualAddress ProcessAddressSpace::addAnonymous(Region region,
                                                             Permissions permissions,
                                                             bool overwriteExisiting,
                                                             bool shared) {
    auto memoryArea = make_shared<MemoryArea>(shared,
                                              Permissions::READ_WRITE_EXECUTE,
                                              region.length);
    return add(region, 0, memoryArea, permissions, overwriteExisiting);
}

UserVirtualAddress ProcessAddressSpace::addAnonymous(size_t length, Permissions permissions) {
    scoped_lock sl(lock);

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    auto [region, index] = findFreeRegion(length);
    auto memoryArea = make_shared<MemoryArea>(false,
                                              Permissions::READ_WRITE_EXECUTE,
                                              region.length);
    auto mappedArea = make_unique<MappedArea>(*this, region, memoryArea, 0, permissions);
    mappedAreas.insert(move(mappedArea), index);

    return region.start;
}

void ProcessAddressSpace::ensureSplitAt(size_t address) {
    auto [index, inUse] = findIndexFor(address);

    if (inUse && mappedAreas[index]->region.start != address) {
        /* area needs to be split */

        /* resize the vector first, since it would be hard to recover from an exeption later */
        mappedAreas.insert(nullptr, index + 1);

        pair<unique_ptr<MappedArea>, unique_ptr<MappedArea>> splitResult;
        try {
            splitResult = mappedAreas[index]->split(address - mappedAreas[index]->region.start);
        } catch (...) {
            /* erase the already reserved vector space for the second element */
            mappedAreas.erase(mappedAreas.begin() + index + 1);
            throw;
        }

        mappedAreas[index] = move(splitResult.first);
        mappedAreas[index + 1] = move(splitResult.second);
    }
}

void ProcessAddressSpace::ensureRegionIndependent(Region region) {
    ensureSplitAt(region.start);
    ensureSplitAt(region.end());
}


void ProcessAddressSpace::_removeRegion(Region region) {
    assert(lock.isLocked());

    ensureRegionIndependent(region);

    auto [startIndex, mapped] = findIndexFor(region.start);
    auto startIterator = mappedAreas.begin() + startIndex;
    auto endIterator = startIterator;
    while (endIterator != mappedAreas.end() && (*endIterator)->region.start < region.end()) {
        endIterator++;
    }

    mappedAreas.erase(startIterator, endIterator);
}

void ProcessAddressSpace::removeRegion(Region region) {
    scoped_lock sl(lock);
    _removeRegion(region);
}

size_t ProcessAddressSpace::changeRegionProtection(Region region, Permissions permissions) {
    scoped_lock sl(lock);

    ensureRegionIndependent(region);

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
void ProcessAddressSpace::removeMapping(size_t startAddress) {
    scoped_lock sl(lock);

    auto [index, mapped] = findIndexFor(startAddress);
    if (!mapped || mappedAreas[index]->region.start != startAddress) {
        cout << "removeMapping: give address " << startAddress << " is not the start of a mapping"
             << endl;
        throw BadUserSpace(process());
    }
    mappedAreas.erase(mappedAreas.begin() + index);
}

void ProcessAddressSpace::handlePageFault(size_t address, Permissions requiredPermissions) {
    scoped_lock sl(lock);

    auto [index, mapped] = findIndexFor(address);
    if (!mapped) {
        cout << "Process tried to access address " << address << " which it has not mapped "
             << endl;
        throw BadUserSpace(process());
    }

    unique_ptr<MappedArea> &mappedArea = mappedAreas[index];

    if (mappedArea->permissions == Permissions::INVALID) {
        cout << "Process tried to access address " << address
             << " which is explicitly access-disabled." << endl;
        throw BadUserSpace(process());
    }

    if (requiredPermissions > mappedArea->permissions) {
        cout << "Process tried to access address " << address << " using permissions "
             << requiredPermissions << " which is mapped with " << mappedArea->permissions << endl;
        throw BadUserSpace(process());
    }

    mappedArea->ensurePagedIn(align(address, PAGE_SIZE, AlignDirection::DOWN));
}

uint64_t ProcessAddressSpace::getFutexID(size_t address) {
    scoped_lock sl(lock);

    optional result = findMemoryArea(address, true);
    if (!result) {
        throw BadUserSpace(process());
    }

    auto [memoryArea, maxRegionInside] = *result;
    return memoryArea->getFutexID(maxRegionInside.start);
}

void ProcessAddressSpace::copyFromLocked(uint8_t *destination, size_t address, size_t length) {
    assert(lock.isLocked());

    while (length > 0) {
        optional result = findMemoryArea(address, false);
        if (!result) {
            cout << "copyFrom: could not find memory area for " << address << endl;
            throw BadUserSpace(process());
        }

        auto [memoryArea, maxRegionInside] = *result;
        size_t accessLength = min(length, maxRegionInside.length);
        memoryArea->copyFrom(destination, maxRegionInside.start, accessLength);

        destination += accessLength;
        address += accessLength;
        length -= accessLength;
    }
}

void ProcessAddressSpace::copyFrom(uint8_t *destination, size_t address, size_t length) {
    scoped_lock sl(lock);
    copyFromLocked(destination, address, length);
}


void ProcessAddressSpace::copyTo(size_t address,
                                 const uint8_t *source,
                                 size_t length,
                                 bool requireWritePermissions) {
    scoped_lock sl(lock);

    while (length > 0) {
        optional result = findMemoryArea(address, requireWritePermissions);
        if (!result) {
            throw BadUserSpace(process());
        }

        auto [memoryArea, maxRegionInside] = *result;
        size_t accessLength = min(length, maxRegionInside.length);
        memoryArea->copyTo(maxRegionInside.start, source, accessLength);

        source += accessLength;
        address += accessLength;
        length -= accessLength;
    }
}

void ProcessAddressSpace::copyFromOhter(size_t destinationAddress,
                                          ProcessAddressSpace &sourceSpace,
                                          size_t sourceAddress,
                                          size_t length,
                                          bool requireWriteAccessToDestination) {
    scoped_lock sl(lock, sourceSpace.lock);

    while (length > 0) {
        optional sourceResult = sourceSpace.findMemoryArea(sourceAddress, false);
        if (!sourceResult) {
            cout << "copyFromUser: Unable to find corresponding MemoryArea in source" << endl;
            throw BadUserSpace(sourceSpace.process());
        }
        auto [sourceArea, sourceMaxRegion] = *sourceResult;

        optional result = findMemoryArea(destinationAddress, requireWriteAccessToDestination);
        if (!result) {
            cout << "copyFromUser: Unable to find corresponding MemoryArea in destination" << endl;
            throw BadUserSpace(process());
        }
        auto [destinationArea, destinationMaxRegion] = *result;

        size_t accessLength = min(length,
                                  min(sourceMaxRegion.length, destinationMaxRegion.length));
        destinationArea->copyFromOther(destinationMaxRegion.start,
                                       *sourceArea,
                                       sourceMaxRegion.start,
                                       accessLength);
        sourceAddress += accessLength;
        destinationAddress += accessLength;
        length -= accessLength;
    }
}


uint32_t ProcessAddressSpace::atomicCopyFrom32(size_t address) {
    scoped_lock sl(lock);

    optional result = findMemoryArea(address, true);
    if (!result) {
        throw BadUserSpace(process());
    }

    auto [memoryArea, maxRegionInside] = *result;
    return memoryArea->atomicCopyFrom32(maxRegionInside.start);
}

/* PhysicalAddress returned for Futex manager use only. May have changed already since
 * ProcessAddressSpace is unlocked! */
bool ProcessAddressSpace::atomicCompareExchange32(size_t address,
                                                              uint32_t expectedValue,
                                                              uint32_t newValue) {
    scoped_lock sl(lock);

    optional result = findMemoryArea(address, true);
    if (!result) {
        throw BadUserSpace(process());
    }

    auto [memoryArea, maxRegionInside] = *result;
    return memoryArea->atomicCompareExchange32(maxRegionInside.start, expectedValue, newValue);
}

shared_ptr<Process> ProcessAddressSpace::process() const noexcept {
    /* hack relies on addressSpace being the first member of Process */
    Process *raw = reinterpret_cast<Process *>(const_cast<ProcessAddressSpace *>(this));
    auto result = raw->self.lock();
    assert(raw == result.get());
    return result;
}
