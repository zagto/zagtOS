#include <paging/MasterPageTable.hpp>
#include <system/System.hpp>
#include <memory/PlatformRegions.hpp>
#include <tasks/Task.hpp>
#include <tasks/MappedArea.hpp>
#include <tasks/Object.hpp>


Task::Task(ELF elf, Thread::Priority initialPrioriy, Object *runMessage) {
    LockHolder lh(pagingLock);

    masterPageTable = new MasterPageTable();
    activate();

    UserVirtualAddress maxEndAddress = 0;

    for (usize index = 0; index < elf.numSegments(); index++) {
        elf::Segment segment = elf.segment(index);
        if (segment.type() == elf::Segment::TYPE_LOAD) {
            Region region = segment.regionInMemory();

            MappedArea *ma = new MappedArea(this, region, segment.permissions());
            mappedAreas.insert(ma);
            ma->mapEverything();

            segment.load();

            if (segment.endAddress().value() > maxEndAddress.value()) {
                maxEndAddress = segment.endAddress();
            }
        }
    }

    // for now just start the heap after one empty page
    heapStart = UserVirtualAddress(align(maxEndAddress.value(),
                                         PAGE_SIZE,
                                         AlignDirection::UP) + PAGE_SIZE);

    mappedAreas.insert(new MappedArea(this, UserStackRegion, Permissions::WRITE));

    usize objSize = runMessage->sizeInMemory();
    Assert(UserStackRegion.end() > objSize);
    usize objAddr = UserStackRegion.end() - objSize;
    while (objAddr % 16) {
        objAddr--;
    }
    Assert(objAddr > UserStackRegion.start);

    for (usize address = objAddr; address + PAGE_SIZE < objAddr + objSize; address+=PAGE_SIZE) {
        handlePageFault(UserVirtualAddress(address));
    }

    UserVirtualAddress masterTLSBase{0};
    usize tlsSize{0};
    if (elf.hasTLS()) {
        tlsSize = elf.tlsSegment().length();
        masterTLSBase = elf.tlsSegment().address();
    }

    bool valid;
    Region tlsRegion = findFreeRegion(tlsSize + THREAD_STRUCT_AREA_SIZE, valid);
    if (!valid) {
        Log << "TODO: whatever should happen if there is no memory" << EndLine;
        Panic();
    }

    MappedArea *tlsArea = new MappedArea(this, tlsRegion, Permissions::WRITE);
    tlsArea->mapEverything();
    mappedAreas.insert(tlsArea);

    UserVirtualAddress tlsBase(tlsRegion.start + THREAD_STRUCT_AREA_SIZE);
    if (elf.hasTLS()) {
        elf.tlsSegment().load(tlsBase);
    }

    Thread *mainThread = new Thread(this,
                                    elf.entry(),
                                    initialPrioriy,
                                    objAddr,
                                    tlsBase,
                                    masterTLSBase,
                                    tlsSize);

    threads.pushBack(mainThread);
    CurrentProcessor->scheduler.add(mainThread);
}

void Task::activate() {
    if (CurrentProcessor->currentTask == this) {
        return;
    }
    masterPageTable->activate();
    CurrentProcessor->currentTask = this;
}

PhysicalAddress Task::allocateFrame(UserVirtualAddress address,
                                    Permissions permissions) {
    Assert(permissions == Permissions::WRITE
           || permissions == Permissions::EXECUTE
           || permissions == Permissions::NONE);

    PhysicalAddress physical = CurrentSystem.memory.allocatePhysicalFrame();
    masterPageTable->map(address, physical, permissions);
    return physical;
}

bool Task::handlePageFault(UserVirtualAddress address) {
    UserVirtualAddress pageAddress{align(address.value(), PAGE_SIZE, AlignDirection::DOWN)};
    LockHolder lh(pagingLock);

    MappedArea *ma = findMappedArea(address);
    if (ma) {
        return ma->handlePageFault(pageAddress);
    } else {
        return false;
    }
}

MappedArea *Task::findMappedArea(UserVirtualAddress address) {
    Assert(pagingLock.isLocked());

    usize low = 0;
    usize high = mappedAreas.size() - 1;
    while (low < high) {
        usize index = (high + low) / 2;
        if (address.isInRegion(mappedAreas[index]->region)) {
            return mappedAreas[index];
        }
        if (address.value() > mappedAreas[index]->region.start) {
            low = index + 1;
        } else {
            high = index - 1;
        }
    }
    return nullptr;
}

Region Task::findFreeRegion(usize length, bool &valid) {
    Assert(pagingLock.isLocked());

    valid = false;

    length = align(length, PAGE_SIZE, AlignDirection::UP);

    usize base = heapStart.value();
    if (mappedAreas.size() && mappedAreas[0]->region.start < base + length) {
        for (usize index = 0; index < mappedAreas.size(); index++) {
            usize nextStart;
            if (index == mappedAreas.size()) {
                nextStart = UserHeapEnd;
            } else {
                nextStart = mappedAreas[index + 1]->region.start;
            }
            if (nextStart > UserHeapEnd) {
                nextStart = UserHeapEnd;
            }

            usize areaEnd = mappedAreas[index]->region.end();
            Assert(nextStart <= areaEnd);

            if (areaEnd >= UserHeapEnd) {
                /* We've failed... there is no big enough memory region in heap area left */
                goto fail;
            }

            if (nextStart - areaEnd < length) {
                base = areaEnd;
                break;
            }
        }
        if (base == heapStart.value()) {
            /* base still has initial non-usable value. nothing found :( */
            goto fail;
        }
    }

    valid = true;
    return Region(base, length);

fail:
    return Region(0, 0);
}


void Task::removeThread(Thread *thread) {
    threads.remove(thread);
}

/*
bool Task::changeHeapSize(isize change, UserVirtualAddress *previousEnd) {
    LockHolder lh(&_pagingLock);

    if (change < 0 && nonAlignedHeapSize < static_cast<usize>(-change)) {
        return false;
    }
    if (change > 0
            && UserStackRegion.start
                - USER_STACK_SIZE
                - USER_STACK_BORDER
                - nonAlignedHeapSize
            < static_cast<usize>(change)) {
        return false;
    }

    *previousEnd = nonAlignedHeapEnd();

    isize alignedChange = align(nonAlignedHeapSize + change,
                                PAGE_SIZE,
                                AlignDirection::UP) - alignedHeapSize;

    while (alignedChange > 0) {
        allocateFrame(alignedHeapEnd(), Permissions::WRITE);
        alignedHeapSize += PAGE_SIZE;
        alignedChange -= PAGE_SIZE;
    }
    while (alignedChange < 0) {
        alignedHeapSize += PAGE_SIZE;
        alignedChange -= PAGE_SIZE;
        _masterPageTable->unmap(alignedHeapEnd());
    }
    nonAlignedHeapSize += change;

    return true;
}
*/
