#include <common/common.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>


MappedArea::MappedArea(Process *process, Region region, Permissions permissions) :
        process{process},
        source{Source::MEMORY},
        region{region},
        permissions{permissions} {
    assert(UserVirtualAddress::checkInRegion(region.start));
    assert(UserVirtualAddress::checkInRegion(region.end() - 1));
    assert(region.isPageAligned());
    assert(permissions != Permissions::WRITE_AND_EXECUTE);
}


MappedArea::MappedArea(Process *process,
                       Region region,
                       Permissions
                       permissions,
                       PhysicalAddress pyhsicalStart) :
        process{process},
        source{Source::PHYSICAL_MEMORY},
        physicalStart{pyhsicalStart},
        region{region},
        permissions{permissions} {
    assert(UserVirtualAddress::checkInRegion(region.start));
    assert(UserVirtualAddress::checkInRegion(region.end() - 1));
    assert(region.isPageAligned());
    assert(permissions != Permissions::WRITE_AND_EXECUTE);
}


MappedArea::~MappedArea() {
    unmapRange(region);
}


bool MappedArea::handlePageFault(UserVirtualAddress address) {
    assert(process->pagingLock.isLocked());
    if (process->masterPageTable->isMapped(address)) {
        process->masterPageTable->invalidateLocally(address);
    } else {
        if (source == Source::MEMORY) {
            process->allocateFrame(address, permissions);
        } else if (source == Source::PHYSICAL_MEMORY) {
            process->masterPageTable->map(address,
                                       physicalStart + (address.value() - region.start),
                                       permissions);
        } else {
            Panic();
        }
    }
    return true;
}

void MappedArea::unmapRange(Region range) {
    assert(range.isPageAligned());

    switch (source) {
    case Source::MEMORY:
        process->masterPageTable->unmapRange(range.start, range.length / PAGE_SIZE, true);
        break;
    case Source::PHYSICAL_MEMORY:
        process->masterPageTable->unmapRange(range.start, range.length / PAGE_SIZE, false);
        break;
    default:
        Panic();
    }
}

void MappedArea::shrinkFront(size_t amount) {
    assert(amount % PAGE_SIZE == 0);

    unmapRange(Region(region.start, amount));
    region.start += amount;
    region.length -= amount;
}

void MappedArea::shrinkBack(size_t amount) {
    assert(amount % PAGE_SIZE == 0);

    unmapRange(Region(region.end() - amount, amount));
    region.length -= amount;
}

bool MappedAreaVector::findMappedAreaIndexOrFreeLength(UserVirtualAddress address,
                                                       size_t &resultIndex,
                                                       size_t &freeLength) {
    assert(process->pagingLock.isLocked());

    if (size() == 0) {
        resultIndex = UserSpaceRegion.end() - address.value();
        return false;
    }

    size_t low = 0;
    size_t high = size() - 1;
    while (low < high) {
        size_t index = low + (high - low) / 2;
        if (address.isInRegion(_data[index]->region)) {
            resultIndex = index;
            freeLength = 0;
            return true;
        }
        if (address.value() < _data[index]->region.start) {
            if (index == low) {
                resultIndex = index;
                freeLength = _data[index]->region.start - address.value();
                return false;
            }
            high = index - 1;
        } else {
            low = index + 1;
        }
    }
    assert(low == high);
    assert(_data[low] != nullptr);
    if (address.isInRegion(_data[low]->region)) {
        resultIndex = low;
        freeLength = 0;
        return true;
    } else {
        if (address.value() < _data[low]->region.start) {
            resultIndex = low;
            freeLength = _data[low]->region.start - address.value();
        } else {
            resultIndex = low + 1;
            if (low == size() - 1) {
                freeLength = UserSpaceRegion.end() - address.value();
            } else {
                freeLength = _data[low+1]->region.start - address.value();
            }
        }
        return false;
    }
}

MappedArea *MappedAreaVector::findMappedArea(UserVirtualAddress address) {
    size_t index, unused;
    bool mapped = findMappedAreaIndexOrFreeLength(address, index, unused);

    if (mapped) {
        return _data[index];
    } else {
        return nullptr;
    }
}

Region MappedAreaVector::findFreeRegion(size_t length, bool &valid, size_t &newIndex) {
    assert(process->pagingLock.isLocked());

    valid = false;
    bool found = false;

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    /* do not use first page to make it easiert to find null pointer dereferences */
    size_t base = PAGE_SIZE;

    if (size() > 0) {
        for (size_t index = 0; index < size(); index++) {
            size_t nextStart;
            if (index == size()) {
                nextStart = UserSpaceRegion.end();
            } else {
                nextStart = _data[index + 1]->region.start;
            }
            if (nextStart > UserSpaceRegion.end()) {
                nextStart = UserSpaceRegion.end();
            }

            size_t areaEnd = _data[index]->region.end();
            assert(nextStart >= areaEnd);
            assert(areaEnd <= UserSpaceRegion.end());

            if (nextStart - areaEnd >= length) {
                found = true;
                base = areaEnd;
                newIndex = index + 1;
                break;
            }
        }
        if (!found) {
            cout << "findFreeRegion: didn't find long enough free region" << endl;
            goto fail;
        }
    } else {
        newIndex = 0;
    }

    valid = true;
    return Region(base, length);

fail:
    newIndex = -1ul;
    return Region(0, 0);
}

void MappedAreaVector::insert2(MappedArea *ma, size_t index) {
    assert(index == this->findIndexFor(ma));
    static_cast<vector<MappedArea *> *>(this)->insert(ma, index);
}

MappedArea *MappedAreaVector::addNew(size_t length, Permissions permissions) {
    bool valid;
    size_t index;
    Region region = findFreeRegion(length, valid, index);
    if (!valid) {
        return nullptr;
    }

    MappedArea *ma = new MappedArea(process, region, permissions);
    insert2(ma, index);
    return ma;
}

bool MappedAreaVector::isRegionFree(Region region, size_t &insertIndex) {
    size_t freeLength;
    bool mapped = findMappedAreaIndexOrFreeLength(UserVirtualAddress(region.start),
                                                  insertIndex,
                                                  freeLength);

    if (mapped) {
        return false;
    } else {
        return region.length <= freeLength;
    }
}

void MappedAreaVector::splitElement(size_t index, Region removeRegion, size_t numAddBetween) {
    assert(index < numElements);
    assert(removeRegion.isPageAligned());
    /* removeRange has to be fully inside the element's region */
    assert(removeRegion.start > _data[index]->region.start);
    assert(removeRegion.end() > _data[index]->region.start);
    assert(removeRegion.start < _data[index]->region.end());
    assert(removeRegion.end() < _data[index]->region.end());

    size_t numMove = numElements - index - 1;

    numElements += numAddBetween + 1;
    adjustAllocatedSize();

    memmove(&_data[index + numAddBetween + 2],
            &_data[index + 1],
            numMove * sizeof(MappedArea *));

    MappedArea &oldElement = *_data[index];
    oldElement.unmapRange(removeRegion);

    size_t newElementIndex = index + numAddBetween + 1;
    Region newElementRegion(removeRegion.end(), oldElement.region.end() - removeRegion.end());
    _data[newElementIndex] = new MappedArea(process, newElementRegion, oldElement.permissions);

    /* oldElement will still be responsible for the first part */
    oldElement.region.length -= removeRegion.start - oldElement.region.end();
    assert(oldElement.region.end() == removeRegion.start);

    for (size_t i = index + 1; i < newElementIndex; i++) {
        _data[i] = nullptr;
    }
}

/*
 * Unmaps all MappedAreas that are inside range. MappedAreas crossing range boundaries will be
 * truncated.
 * The method will add numAddInstead of new slots for MappedAreas to the vector in place of the
 * removed range. No MappedArea object will be created for these, they will be initialized to
 * nullptr. The index of the first of theses slots is returned
 */
size_t MappedAreaVector::unmapRange(Region range, size_t numAddInstead) {
    size_t index;
    if (!isRegionFree(range, index)) {
        assert(index < numElements);

        MappedArea &firstMA = *_data[index];

        if (firstMA.region.start < range.start) {
            /* first area overlaps the start of range to unmap */
            assert(firstMA.region.end() > range.start);

            if (firstMA.region.end() > range.end()) {
                /* range is fully contained in mapped area. -> split it! */
                splitElement(index, range, numAddInstead);
                return index + 1;
            }

            firstMA.shrinkBack(firstMA.region.end() - range.start);
            assert(firstMA.region.end() == range.start);

            /* exclude the first area from being unmapped as a whole */
            index++;
        }
    }
    size_t endIndex = index;

    while (endIndex < numElements && _data[endIndex]->region.end() <= range.end()) {
        delete _data[endIndex];
        _data[endIndex] = nullptr;

        endIndex++;
    }

    if (endIndex < numElements && _data[endIndex]->region.start < range.end()) {
        /* MappedArea at endIndex is partially in range */
        _data[endIndex]->shrinkFront(range.end() - _data[endIndex]->region.start);
    }

    size_t numMove = numElements - endIndex;

    numElements = numElements - (endIndex - index) + numAddInstead;
    if (numAddInstead > endIndex - index) {
        adjustAllocatedSize();
    }
    memmove(&_data[index + numAddInstead],
            &_data[endIndex],
            numMove * sizeof(MappedArea *));
    adjustAllocatedSize();

    for (size_t i = index; i < index + numAddInstead; i++) {
        _data[i] = nullptr;
    }

    return index;
}
