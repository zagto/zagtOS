#include <common/common.hpp>
#include <tasks/MappedArea.hpp>
#include <tasks/Task.hpp>


MappedArea::MappedArea(Task *task, Region region, Permissions permissions) :
        task{task},
        source{Source::MEMORY},
        region{region},
        permissions{permissions} {
    assert(UserVirtualAddress::checkInRegion(region.start));
    assert(UserVirtualAddress::checkInRegion(region.end() - 1));
    assert(region.isPageAligned());
    assert(permissions != Permissions::WRITE_AND_EXECUTE);
}


MappedArea::MappedArea(Task *task,
                       Region region,
                       Permissions
                       permissions,
                       PhysicalAddress pyhsicalStart) :
        task{task},
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
    assert(task->pagingLock.isLocked());
    if (task->masterPageTable->isMapped(address)) {
        task->masterPageTable->invalidateLocally(address);
    } else {
        if (source == Source::MEMORY) {
            task->allocateFrame(address, permissions);
        } else if (source == Source::PHYSICAL_MEMORY) {
            task->masterPageTable->map(address,
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
        task->masterPageTable->unmapRange(range.start, range.length / PAGE_SIZE, true);
        break;
    case Source::PHYSICAL_MEMORY:
        task->masterPageTable->unmapRange(range.start, range.length / PAGE_SIZE, true);
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
    assert(task->pagingLock.isLocked());

    if (size() == 0) {
        resultIndex = UserSpaceRegion.end() - address.value();
        return false;
    }

    size_t low = 0;
    size_t high = size() - 1;
    while (low < high) {
        size_t index = low + (high - low) / 2;
        if (address.isInRegion(data[index]->region)) {
            resultIndex = index;
            freeLength = 0;
            return true;
        }
        if (address.value() < data[index]->region.start) {
            if (index == low) {
                resultIndex = index;
                freeLength = data[index]->region.start - address.value();
                return false;
            }
            high = index - 1;
        } else {
            low = index + 1;
        }
    }
    assert(low == high);
    assert(data[low] != nullptr);
    if (address.isInRegion(data[low]->region)) {
        resultIndex = low;
        freeLength = 0;
        return true;
    } else {
        if (address.value() < data[low]->region.start) {
            resultIndex = low;
            freeLength = data[low]->region.start - address.value();
        } else {
            resultIndex = low + 1;
            if (low == size() - 1) {
                freeLength = UserSpaceRegion.end() - address.value();
            } else {
                freeLength = data[low+1]->region.start - address.value();
            }
        }
        return false;
    }
}

MappedArea *MappedAreaVector::findMappedArea(UserVirtualAddress address) {
    size_t index, unused;
    bool mapped = findMappedAreaIndexOrFreeLength(address, index, unused);

    if (mapped) {
        return data[index];
    } else {
        return nullptr;
    }
}

Region MappedAreaVector::findFreeRegion(size_t length, bool &valid, size_t &newIndex) {
    assert(task->pagingLock.isLocked());

    valid = false;
    bool found = false;

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    size_t base = task->heapStart.value();

    if (size() > 0) {
        for (size_t index = 0; index < size(); index++) {
            size_t nextStart;
            if (index == size()) {
                nextStart = UserSpaceRegion.end();
            } else {
                nextStart = data[index + 1]->region.start;
            }
            if (nextStart > UserSpaceRegion.end()) {
                nextStart = UserSpaceRegion.end();
            }

            size_t areaEnd = data[index]->region.end();
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

    MappedArea *ma = new MappedArea(task, region, permissions);
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
    assert(removeRegion.start > data[index]->region.start);
    assert(removeRegion.end() > data[index]->region.start);
    assert(removeRegion.start < data[index]->region.end());
    assert(removeRegion.end() < data[index]->region.end());

    size_t numMove = numElements - index - 1;

    numElements += numAddBetween + 1;
    updateAllocatedSize();

    memmove(&data[index + numAddBetween + 2],
            &data[index + 1],
            numMove * sizeof(MappedArea *));

    MappedArea &oldElement = *data[index];
    oldElement.unmapRange(removeRegion);

    size_t newElementIndex = index + numAddBetween + 1;
    Region newElementRegion(removeRegion.end(), oldElement.region.end() - removeRegion.end());
    data[newElementIndex] = new MappedArea(task, newElementRegion, oldElement.permissions);

    /* oldElement will still be responsible for the first part */
    oldElement.region.length -= removeRegion.start - oldElement.region.end();
    assert(oldElement.region.end() == removeRegion.start);

    for (size_t i = index + 1; i < newElementIndex; i++) {
        data[i] = nullptr;
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

        MappedArea &firstMA = *data[index];

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

    while (endIndex < numElements && data[endIndex]->region.end() <= range.end()) {
        delete data[endIndex];
        data[endIndex] = nullptr;

        endIndex++;
    }

    if (endIndex < numElements && data[endIndex]->region.start < range.end()) {
        /* MappedArea at endIndex is partially in range */
        data[endIndex]->shrinkFront(range.end() - data[endIndex]->region.start);
    }

    size_t numMove = numElements - endIndex;

    numElements = numElements - (endIndex - index) + numAddInstead;
    if (numAddInstead > endIndex - index) {
        updateAllocatedSize();
    }
    memmove(&data[index + numAddInstead],
            &data[endIndex],
            numMove * sizeof(MappedArea *));
    updateAllocatedSize();

    for (size_t i = index; i < index + numAddInstead; i++) {
        data[i] = nullptr;
    }

    return index;
}
