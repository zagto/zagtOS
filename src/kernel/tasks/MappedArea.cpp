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

MappedArea *MappedAreaVector::findMappedArea(UserVirtualAddress address) {
    assert(task->pagingLock.isLocked());

    if (size() == 0) {
        return nullptr;
    }

    size_t low = 0;
    size_t high = size() - 1;
    while (low < high) {
        size_t index = low + (high - low) / 2;
        if (address.isInRegion(data[index]->region)) {
            cout << "found\n";
            return data[index];
        }
        if (address.value() < data[index]->region.start) {
            if (index == low) {
                return nullptr;
            }
            cout << "left\n";
            high = index - 1;
        } else {
            cout << "right\n";
            low = index + 1;
        }
    }
    assert(low == high);
    if (address.isInRegion(data[low]->region)) {
        return data[low];
    } else {
        return nullptr;
    }
}

Region MappedAreaVector::findFreeRegion(size_t length, bool &valid, size_t &newIndex) {
    assert(task->pagingLock.isLocked());

    valid = false;

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    size_t base = task->heapStart.value();

    /* if region does not fit directly at start */
    if (size() > 0 && data[0]->region.start < base + length) {
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

            if (areaEnd >= UserSpaceRegion.end()) {
                /* We've failed... there is no big enough memory region in heap area left */
                goto fail;
            }

            if (nextStart - areaEnd >= length) {
                base = areaEnd;
                newIndex = index + 1;
                break;
            }
        }
        if (base == task->heapStart.value()) {
            /* base still has initial non-usable value. nothing found :( */
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

MappedArea *MappedAreaVector::addNew(size_t length, Permissions permissions) {
    bool valid;
    size_t index;
    Region region = findFreeRegion(length, valid, index);
    if (!valid) {
        return nullptr;
    }

    MappedArea *ma = new MappedArea(task, region, permissions);
    cout << "index from findFreeRegion: " << index << "\n";
    cout << "index from findIndexFor: " << this->findIndexFor(ma) << "\n";
    assert(index == this->findIndexFor(ma));

    static_cast<vector<MappedArea *> *>(this)->insert(ma, index);
    return ma;
}

