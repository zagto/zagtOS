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


bool MappedArea::handlePageFault(UserVirtualAddress address) {
    if (source == Source::MEMORY) {
        assert(task->pagingLock.isLocked());
        if (task->masterPageTable->isMapped(address)) {
            task->masterPageTable->invalidateLocally(address);
        } else {
            task->allocateFrame(address, permissions);
        }
        return true;
    } else {
        Panic();
    }
}

void MappedArea::mapEverything() {
    assert(source == Source::MEMORY);

    for (size_t address = region.start; address < region.length; address += PAGE_SIZE) {
        if (!task->masterPageTable->isMapped(address)) {
            task->masterPageTable->invalidateLocally(UserVirtualAddress(address));
        }
    }
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
    cout << "heapStart: " << base << endl;

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
                cout << "findFreeRegion: found " << areaEnd << endl;
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
