#include <common/common.hpp>
#include <tasks/MappedArea.hpp>
#include <tasks/Task.hpp>


MappedArea::MappedArea(Task *task, Region region, Permissions permissions) :
        task{task},
        source{Source::MEMORY},
        region{region},
        permissions{permissions} {
    Assert(UserVirtualAddress::checkInRegion(region.start));
    Assert(UserVirtualAddress::checkInRegion(region.end() - 1));
    Assert(region.isPageAligned());
    Assert(permissions != Permissions::WRITE_AND_EXECUTE);
}


bool MappedArea::handlePageFault(UserVirtualAddress address) {
    if (source == Source::MEMORY) {
        Assert(task->pagingLock.isLocked());
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
    Assert(source == Source::MEMORY);

    for (usize address = region.start; address < region.length; address += PAGE_SIZE) {
        if (!task->masterPageTable->isMapped(address)) {
            task->masterPageTable->invalidateLocally(UserVirtualAddress(address));
        }
    }
}

MappedArea *MappedAreaVector::findMappedArea(UserVirtualAddress address) {
    Assert(task->pagingLock.isLocked());

    if (size() == 0) {
        return nullptr;
    }

    usize low = 0;
    usize high = size() - 1;
    while (low < high) {
        usize index = low + (high - low) / 2;
        if (address.isInRegion(data[index]->region)) {
            Log << "found\n";
            return data[index];
        }
        if (address.value() < data[index]->region.start) {
            if (index == low) {
                return nullptr;
            }
            Log << "left\n";
            high = index - 1;
        } else {
            Log << "right\n";
            low = index + 1;
        }
    }
    Assert(low == high);
    if (address.isInRegion(data[low]->region)) {
        return data[low];
    } else {
        return nullptr;
    }
}

Region MappedAreaVector::findFreeRegion(usize length, bool &valid, usize &newIndex) {
    Assert(task->pagingLock.isLocked());

    valid = false;

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    usize base = task->heapStart.value();

    /* if region does not fit directly at start */
    if (size() > 0 && data[0]->region.start < base + length) {
        for (usize index = 0; index < size(); index++) {
            usize nextStart;
            if (index == size()) {
                nextStart = UserSpaceRegion.end();
            } else {
                nextStart = data[index + 1]->region.start;
            }
            if (nextStart > UserSpaceRegion.end()) {
                nextStart = UserSpaceRegion.end();
            }

            usize areaEnd = data[index]->region.end();
            Assert(nextStart >= areaEnd);

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

MappedArea *MappedAreaVector::addNew(usize length, Permissions permissions) {
    bool valid;
    usize index;
    Region region = findFreeRegion(length, valid, index);
    if (!valid) {
        return nullptr;
    }

    MappedArea *ma = new MappedArea(task, region, permissions);
    Log << "index from findFreeRegion: " << index << "\n";
    Log << "index from findIndexFor: " << this->findIndexFor(ma) << "\n";
    Assert(index == this->findIndexFor(ma));

    static_cast<Vector<MappedArea *> *>(this)->insert(ma, index);
    return ma;
}

