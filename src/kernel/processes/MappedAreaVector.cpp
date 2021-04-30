#include <processes/MappedAreaVector.hpp>

MappedAreaVector::MappedAreaVector(Process *process,
                                   const vector<shared_ptr<MemoryArea> > &allMemoryAreas,
                                   const hos_v1::Process &handOver,
                                   Status &status) :
        process{process} {
    if (!status) {
        return;
    }

    status = resize(handOver.numMappedAreas);
    if (!status) {
        return;
    }

    for (size_t index = 0; index < handOver.numMappedAreas; index++) {
        Result result = make_raw<MappedArea>(process,
                                      allMemoryAreas[handOver.mappedAreas[index].memoryAreaID],
                                      handOver.mappedAreas[index]);
        assert(result); // TODO: this code will be replaced anyways
        _data[index] = *result;
        assert(_data[index]->region.isPageAligned());
        if (index > 0) {
            assert(_data[index]->region.start >= _data[index - 1]->region.end());
        }
    }
}


bool MappedAreaVector::findMappedAreaIndexOrFreeLength(UserVirtualAddress address,
                                                       size_t &resultIndex,
                                                       size_t &freeLength) const {
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

MappedArea *MappedAreaVector::findMappedArea(UserVirtualAddress address) const {
    size_t index, unused;
    bool mapped = findMappedAreaIndexOrFreeLength(address, index, unused);

    if (mapped) {
        return _data[index];
    } else {
        return nullptr;
    }
}

Region MappedAreaVector::findFreeRegion(size_t length, bool &valid, size_t &newIndex) const {
    assert(process->pagingLock.isLocked());

    valid = false;
    bool found = false;

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    // do not use first page to make it easiert to find null pointer dereferences
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
    assert(ma != nullptr);
    Status status = static_cast<vector<MappedArea *> *>(this)->insert(ma, index);
    assert(status); // TODO
}

Result<MappedArea *> MappedAreaVector::addNew(Region region, Permissions permissions) {
    size_t index;
    if (!isRegionFree(region, index)) {
        cout << "Attempt to add region to MappedAreaVector that overlaps with existing" << endl;
        return Status::BadUserSpace();
    }

    Result memoryArea = make_shared<MemoryArea>(region.length);
    if (!memoryArea) {
        return memoryArea.status();
    }

    Result ma = make_raw<MappedArea>(process, region, *memoryArea, 0, permissions);
    if (!ma) {
        return ma.status();
    }

    insert2(*ma, index);
    return *ma;
}

Result<MappedArea *> MappedAreaVector::addNew(size_t length, Permissions permissions) {
    bool valid;
    size_t index;
    Region region = findFreeRegion(length, valid, index);
    if (!valid) {
        return nullptr;
    }

    Result memoryArea = make_shared<MemoryArea>(region.length);
    if (!memoryArea) {
        return memoryArea.status();
    }

    Result ma = make_raw<MappedArea>(process, region, *memoryArea, 0, permissions);
    if (!ma) {
        return ma.status();
    }

    insert2(*ma, index);
    return *ma;
}

bool MappedAreaVector::isRegionFree(Region region, size_t &insertIndex) const {
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
    // removeRange has to be fully inside the element's region
    assert(removeRegion.start > _data[index]->region.start);
    assert(removeRegion.end() > _data[index]->region.start);
    assert(removeRegion.start < _data[index]->region.end());
    assert(removeRegion.end() < _data[index]->region.end());

    size_t numMove = numElements - index - 1;


    Status status = adjustAllocatedSize(numElements += numAddBetween + 1);
    assert(status); // TODO

    memmove(&_data[index + numAddBetween + 2],
            &_data[index + 1],
            numMove * sizeof(MappedArea *));

    MappedArea &oldElement = *_data[index];
    oldElement.unmapRange(removeRegion);

    size_t newElementIndex = index + numAddBetween + 1;
    Region newElementRegion(removeRegion.end(), oldElement.region.end() - removeRegion.end());

    cout << "TODO\n";
    Panic();

    //_data[newElementIndex] = *make_raw<MappedArea>(oldElement); // TODO
    _data[newElementIndex]->region = newElementRegion;

    // oldElement will still be responsible for the first part
    oldElement.region.length -= oldElement.region.end() - removeRegion.start;
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
Result<size_t> MappedAreaVector::unmapRange(Region range, size_t numAddInstead) {
    size_t index;
    if (!isRegionFree(range, index)) {
        assert(index < numElements);

        MappedArea &firstMA = *_data[index];

        if (firstMA.region.start < range.start) {
            // first area overlaps the start of range to unmap
            assert(firstMA.region.end() > range.start);

            if (firstMA.region.end() > range.end()) {
                // range is fully contained in mapped area. -> split it!
                splitElement(index, range, numAddInstead);
                return index + 1;
            }

            firstMA.shrinkBack(firstMA.region.end() - range.start);
            assert(firstMA.region.end() == range.start);

            // exclude the first area from being unmapped as a whole
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
        // MappedArea at endIndex is partially in range
        _data[endIndex]->shrinkFront(range.end() - _data[endIndex]->region.start);
    }

    size_t numMove = numElements - endIndex;

    if (numAddInstead > endIndex - index) {
        Status status = adjustAllocatedSize(numElements - (endIndex - index) + numAddInstead);
        if (!status) {
            cout << "Oh no but this will be changed anyways" << endl;
        }
    }
    memmove(&_data[index + numAddInstead],
            &_data[endIndex],
            numMove * sizeof(MappedArea *));
    Status status = adjustAllocatedSize(numElements - (endIndex - index) + numAddInstead);
    assert(status);

    for (size_t i = index; i < index + numAddInstead; i++) {
        _data[i] = nullptr;
    }

    return index;
}


bool MappedAreaVector::isRegionFullyMapped(Region range, size_t &index) const {
    if (isRegionFree(range, index)) {
        return false;
    }

    assert(index < numElements);

    MappedArea &firstMA = *_data[index];
    if (firstMA.region.start > range.start) {
        /* unmapped at start */
        return false;
    }

    size_t endIndex = index + 1;
    while (endIndex < numElements && _data[endIndex]->region.end() <= range.end()) {
        if (_data[endIndex]->region.start != _data[endIndex-1]->region.end()) {
            /* gap */
            return false;
        }
        endIndex++;
    }

    if (_data[endIndex-1]->region.end() < range.end()) {
        /* unmapped at end */
        return false;
    }

    return true;
}



/*
 * Unmaps all MappedAreas that are inside range. MappedAreas crossing range boundaries will be
 * truncated.
 * The method will add numAddInstead of new slots for MappedAreas to the vector in place of the
 * removed range. No MappedArea object will be created for these, they will be initialized to
 * nullptr. The index of the first of theses slots is returned
 */
bool MappedAreaVector::changeRangePermissions(Region range, Permissions newPermissions) {
    size_t index;
    if (!isRegionFullyMapped(range, index)) {
        cout << "warning: attempt to change permissions of area that is not fully mapped" << endl;
        return false;
    }

    assert(index < numElements);

    MappedArea &firstMA = *_data[index];

    if (firstMA.region.start < range.start) {
        /* first area overlaps the start of range to reprotect */
        assert(firstMA.region.end() > range.start);

        splitElement(index, Region(range.start, 0), 0);

        if (firstMA.region.end() > range.end()) {
            /* range is fully contained in mapped area. -> split it twice! */
            splitElement(index + 1, Region(range.end(), 0), 0);
            _data[index + 1]->changePermissions(newPermissions);
            return true;
        }

        _data[index + 1]->changePermissions(newPermissions);
        assert(firstMA.region.end() == range.start);

        /* exclude the first area from being changed as a whole */
        index++;
    }
    size_t endIndex = index;

    while (endIndex < numElements && _data[endIndex]->region.end() <= range.end()) {
        _data[endIndex]->changePermissions(newPermissions);
        endIndex++;
    }

    if (endIndex < numElements && _data[endIndex]->region.start < range.end()) {
        /* MappedArea at endIndex is partially in range */
        splitElement(endIndex, Region(range.end(), 0), 0);
        _data[endIndex + 1]->changePermissions(newPermissions);
    }

    return true;
}

