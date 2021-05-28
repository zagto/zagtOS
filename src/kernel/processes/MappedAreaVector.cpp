#include <processes/MappedAreaVector.hpp>
#include <processes/MappedArea.hpp>

MappedAreaVector::MappedAreaVector(const vector<shared_ptr<MemoryArea> > &allMemoryAreas,
                                   const hos_v1::Process &handOver,
                                   Status &status) {
    if (!status) {
        return;
    }

    status = resize(handOver.numMappedAreas);
    if (!status) {
        return;
    }

    for (size_t index = 0; index < handOver.numMappedAreas; index++) {
        Result result = make_raw<MappedArea>(allMemoryAreas[handOver.mappedAreas[index].memoryAreaID],
                                             handOver.mappedAreas[index]);
        if (!result) {
            cout << "failure during handover" << endl;
        }
        _data[index] = *result;
        assert(_data[index]->region.isPageAligned());
        if (index > 0) {
            assert(_data[index]->region.start >= _data[index - 1]->region.end());
        }
    }
}

optional<MappedArea *> MappedAreaVector::findMappedArea(UserVirtualAddress address) const {
}

Region MappedAreaVector::findFreeRegion(size_t length, bool &valid, size_t &newIndex) const {
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

/* Unmaps all MappedAreas that are inside range. MappedAreas crossing range boundaries will be
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

