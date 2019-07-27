#include <paging/MasterPageTable.hpp>
#include <system/System.hpp>
#include <memory/PlatformRegions.hpp>
#include <tasks/Task.hpp>
#include <tasks/MappedArea.hpp>
#include <tasks/Object.hpp>


Task::Task(ELF elf, Thread::Priority initialPrioriy, Object *runMessage):
        mappedAreas(this) {
    LockHolder lh(pagingLock);

    masterPageTable = new MasterPageTable();
    activate();

    UserVirtualAddress maxEndAddress = 0;

    for (size_t index = 0; index < elf.numSegments(); index++) {
        elf::Segment segment = elf.segment(index);
        if (segment.type() == elf::Segment::TYPE_LOAD) {
            cout << "A Segment Type: " << (uint64_t)segment.type() << "\n";
            Region region = segment.regionInMemory();

            MappedArea *ma = new MappedArea(this, region, segment.permissions());
            mappedAreas.insert(ma);

            cout << "Mapped areas: \n";
            for (size_t i = 0; i < mappedAreas.size(); i++) {
                cout << mappedAreas[i]->region.start << ", size " << mappedAreas[i]->region.length << "\n";
            }

            segment.load(this);

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

    size_t objSize = runMessage->sizeInMemory();
    Assert(UserStackRegion.end() > objSize);
    size_t objAddr = UserStackRegion.end() - objSize;
    while (objAddr % 16) {
        objAddr--;
    }
    Assert(objAddr > UserStackRegion.start);

    for (size_t address = objAddr; address + PAGE_SIZE < objAddr + objSize; address+=PAGE_SIZE) {
        handlePageFault(UserVirtualAddress(address));
    }

    UserVirtualAddress masterTLSBase{0};
    size_t tlsSize{0};
    if (elf.hasTLS()) {
        tlsSize = elf.tlsSegment().length();
        masterTLSBase = elf.tlsSegment().address();
    }

    MappedArea *tlsArea = mappedAreas.addNew(tlsSize + THREAD_STRUCT_AREA_SIZE, Permissions::WRITE);
    if (tlsArea == nullptr) {
        cout << "TODO: whatever should happen if there is no memory" << endl;
        Panic();
    }

    tlsArea->mapEverything();
    mappedAreas.insert(tlsArea);

    UserVirtualAddress tlsBase(tlsArea->region.start + THREAD_STRUCT_AREA_SIZE);
    if (elf.hasTLS()) {
        elf.tlsSegment().load(this, tlsBase);
    }

    Thread *mainThread = new Thread(this,
                                    elf.entry(),
                                    initialPrioriy,
                                    objAddr,
                                    tlsBase,
                                    masterTLSBase,
                                    tlsSize);

    threads.push_back(mainThread);
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

    MappedArea *ma = mappedAreas.findMappedArea(address);
    cout << "found mapped area\n";
    if (ma) {
        return ma->handlePageFault(pageAddress);
    } else {
        return false;
    }
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
