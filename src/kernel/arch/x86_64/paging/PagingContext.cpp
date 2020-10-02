#include <common/common.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <processes/Process.hpp>


extern "C" void basicInvalidate(VirtualAddress address);


PageTableEntry *PagingContext::partialWalkEntries(VirtualAddress address,
                                                  MissingStrategy missingStrategy,
                                                  size_t startLevel,
                                                  WalkData &data) {
    assert(!address.isInRegion(IdentityMapping));

    for (size_t level = startLevel; level > 0; level--) {
        PageTableEntry *entry = data.tables[level]->entryFor(address, level);

        if (!entry->present()) {
            switch (missingStrategy) {
            case MissingStrategy::NONE:
                cout << "non-present entry during resolve which should not have happened\n";
                Panic();
            case MissingStrategy::RETURN_NULLPTR:
                while (level > 0) {
                    data.tables[level - 1] = nullptr;
                    level--;
                }
                return nullptr;
            case MissingStrategy::CREATE: {
                PhysicalAddress frame = CurrentSystem.memory.allocatePhysicalFrame();
                *entry = PageTableEntry(frame,
                                        Permissions::READ_WRITE_EXECUTE,
                                        true,
                                        CacheType::NORMAL_WRITE_BACK);
                break;
            }
            }
        }

        data.tables[level-1] = entry->addressValue().identityMapped().asPointer<PageTable>();
    }
    return data.tables[0]->entryFor(address, 0);
}


PageTableEntry *PagingContext::walkEntries(VirtualAddress address, MissingStrategy ms) {
    WalkData walkData(masterPageTable);
    return partialWalkEntries(address, ms, MASTER_LEVEL, walkData);
}


PagingContext::PagingContext(Process *process) :
    PagingContext(process, CurrentSystem.memory.allocatePhysicalFrame()) {

    assert(this != CurrentProcessor->activePagingContext);

    for (size_t index = KERNEL_ENTRIES_OFFSET;
         index < PageTable::NUM_ENTRIES - 1;
         index++) {
         masterPageTable->entries[index] = CurrentSystem.kernelOnlyPagingContext.masterPageTable->entries[index];
    }
}


PagingContext::PagingContext(Process *process, PhysicalAddress masterPageTableAddress) :
        process{process},
        masterPageTableAddress{masterPageTableAddress},
        masterPageTable{masterPageTableAddress.identityMapped().asPointer<PageTable>()} {
}


void PagingContext::map(KernelVirtualAddress from,
                          PhysicalAddress to,
                          Permissions permissions,
                          CacheType cacheType) {
    assert(CurrentSystem.memory.kernelPagingLock.isLocked());

    PageTableEntry *entry = CurrentSystem.kernelOnlyPagingContext.walkEntries(from, MissingStrategy::CREATE);
    assert(!entry->present());

    *entry = PageTableEntry(to, permissions, false, cacheType);
    basicInvalidate(from);
}


PhysicalAddress PagingContext::resolve(KernelVirtualAddress address) {
    assert(CurrentSystem.memory.kernelPagingLock.isLocked());

    size_t offset = address.value() % PAGE_SIZE;
    return CurrentSystem.kernelOnlyPagingContext.walkEntries(
                address - offset,
                MissingStrategy::NONE)->addressValue() + offset;
}


void PagingContext::unmap(KernelVirtualAddress address) {
    assert(CurrentSystem.memory.kernelPagingLock.isLocked());

    PageTableEntry *entry = CurrentSystem.kernelOnlyPagingContext.walkEntries(address, MissingStrategy::NONE);
    *entry = PageTableEntry();
}


bool PagingContext::isMapped(KernelVirtualAddress address) {
    assert(CurrentSystem.memory.kernelPagingLock.isLocked());

    PageTableEntry *entry = CurrentSystem.kernelOnlyPagingContext.walkEntries(address, MissingStrategy::RETURN_NULLPTR);
    return entry != nullptr && entry->present();
}


PhysicalAddress PagingContext::resolve(UserVirtualAddress address) {
    assert(process == nullptr || process->pagingLock.isLocked());

    size_t offset = address.value() % PAGE_SIZE;
    return walkEntries(address - offset, MissingStrategy::NONE)->addressValue() + offset;
}


void PagingContext::accessRange(UserVirtualAddress address,
                                  size_t numPages,
                                  size_t startOffset,
                                  size_t endOffset,
                                  uint8_t *buffer,
                                  AccessOperation accOp,
                                  Permissions newPagesPermissions) {
    assert(process == nullptr || process->pagingLock.isLocked());
    assert(address.isPageAligned());
    assert(startOffset < PAGE_SIZE);
    assert(endOffset < PAGE_SIZE);
    assert(numPages > 1 || startOffset + endOffset < PAGE_SIZE);

    if (numPages == 0) {
        return;
    }

    WalkData walkData(masterPageTable);
    size_t indexes[NUM_LEVELS];

    for (size_t i = 0; i < NUM_LEVELS; i++) {
        indexes[i] = PageTable::indexFor(address, i);
    }

    size_t changedLevel = MASTER_LEVEL;

    while (true) {
        PageTableEntry *entry = partialWalkEntries(address,
                                                   MissingStrategy::CREATE,
                                                   changedLevel,
                                                   walkData);
        if (!entry->present()) {
            PhysicalAddress frame = CurrentSystem.memory.allocatePhysicalFrame();
            *entry = PageTableEntry(frame, newPagesPermissions, true, CacheType::NORMAL_WRITE_BACK);
        }

        size_t lengthInPage = PAGE_SIZE - startOffset;
        if (numPages == 1) {
            lengthInPage -= endOffset;
        }

        uint8_t *dataPtr = entry->addressValue().identityMapped().asPointer<uint8_t>() + startOffset;
        switch (accOp) {
        case AccessOperation::READ:
            memcpy(buffer, dataPtr, lengthInPage);
            break;
        case AccessOperation::WRITE:
            memcpy(dataPtr, buffer, lengthInPage);
            break;
        default:
            cout << "accessRange: invalid operation\n";
            Panic();
        }

        /* set startOffset to zero so it is only applied on first page */
        startOffset = 0;
        buffer += lengthInPage;

        indexes[0]++;
        changedLevel = 0;
        while (indexes[changedLevel] == PageTable::NUM_ENTRIES) {
            assert(changedLevel < MASTER_LEVEL);
            indexes[changedLevel] = 0;
            indexes[changedLevel + 1]++;
            changedLevel++;
        }
        numPages--;
        if (numPages == 0) {
            return;
        }
        address = address.value() + PAGE_SIZE;
    }
}


void PagingContext::unmapRange(UserVirtualAddress address, size_t numPages, bool freeFrames) {
    assert(process == nullptr || process->pagingLock.isLocked());
    assert(address.isPageAligned());

    if (numPages == 0) {
        return;
    }

    WalkData walkData(masterPageTable);
    size_t indexes[NUM_LEVELS];

    for (size_t i = 0; i < NUM_LEVELS; i++) {
        indexes[i] = PageTable::indexFor(address, i);
    }

    /* TODO: check for entries before the index, and unmap the table
     * itself if there is none. Currently we may be leaking memory */

    size_t changedLevel = MASTER_LEVEL;

    while (true) {
        PageTableEntry *entry = partialWalkEntries(address,
                                                   MissingStrategy::CREATE,
                                                   changedLevel,
                                                   walkData);

        if (entry != nullptr && entry->present()) {
            if (freeFrames) {
                CurrentSystem.memory.freePhysicalFrame(entry->addressValue());
            }
            *entry = PageTableEntry();
            invalidateLocally(address);
        }

        /* TODO: jumps can be made in entry == nullptr case */
        changedLevel = 0;
        while (indexes[changedLevel] == PageTable::NUM_ENTRIES) {
            assert(changedLevel < MASTER_LEVEL);
            indexes[changedLevel] = 0;
            indexes[changedLevel + 1]++;
            changedLevel++;
        }
        numPages--;
        if (numPages == 0) {
            return;
        }
        address = address.value() + PAGE_SIZE;
    }
}


void PagingContext::changeRangePermissions(UserVirtualAddress address, size_t numPages, Permissions newPermissions) {
    assert(process == nullptr || process->pagingLock.isLocked());
    assert(address.isPageAligned());

    if (numPages == 0) {
        return;
    }

    WalkData walkData(masterPageTable);
    size_t indexes[NUM_LEVELS];

    for (size_t i = 0; i < NUM_LEVELS; i++) {
        indexes[i] = PageTable::indexFor(address, i);
    }

    size_t changedLevel = MASTER_LEVEL;

    while (true) {
        PageTableEntry *entry = partialWalkEntries(address,
                                                   MissingStrategy::CREATE,
                                                   changedLevel,
                                                   walkData);

        if (entry != nullptr && entry->present()) {
            entry->setPermissions(newPermissions);
        }

        /* TODO: jumps can be made in entry == nullptr case */
        changedLevel = 0;
        while (indexes[changedLevel] == PageTable::NUM_ENTRIES) {
            assert(changedLevel < MASTER_LEVEL);
            indexes[changedLevel] = 0;
            indexes[changedLevel + 1]++;
            changedLevel++;
        }
        numPages--;
        if (numPages == 0) {
            return;
        }
        address = address.value() + PAGE_SIZE;
    }
}




void PagingContext::map(UserVirtualAddress from,
                        PhysicalAddress to,
                        Permissions permissions) {
    assert(process == nullptr || process->pagingLock.isLocked());

    PageTableEntry *entry = walkEntries(from, MissingStrategy::CREATE);
    assert(!entry->present());

    *entry = PageTableEntry(to, permissions, true, CacheType::NORMAL_WRITE_BACK);
    if (isActive()) {
        basicInvalidate(from);
    }
}


void PagingContext::unmap(UserVirtualAddress address) {
    assert(process == nullptr || process->pagingLock.isLocked());

    PageTableEntry *entry = walkEntries(address, MissingStrategy::NONE);
    *entry = PageTableEntry();
}


bool PagingContext::isMapped(UserVirtualAddress address) {
    assert(process == nullptr || process->pagingLock.isLocked());

    PageTableEntry *entry = walkEntries(address, MissingStrategy::RETURN_NULLPTR);
    return entry != nullptr && entry->present();
}


void PagingContext::invalidateLocally(KernelVirtualAddress address) {
    basicInvalidate(address);
}

void PagingContext::invalidateLocally(UserVirtualAddress address) {
    assert(isActive());
    assert(process == nullptr || process->pagingLock.isLocked());

    basicInvalidate(address);
}

bool PagingContext::isActive() {
    /* Before the Procssor Object is created, there is only the "kernel-only" paging context that
     * was actually created by the bootloader */
    if (__builtin_expect(CurrentProcessor == nullptr, false)) {
        return this == &CurrentSystem.kernelOnlyPagingContext;
    } else {
        return CurrentProcessor->activePagingContext == this;
    }
}

extern "C" void basicSwitchMasterPageTable(PhysicalAddress address);

void PagingContext::activate() {
    if (!isActive()) {
        CurrentProcessor->activePagingContext = this;
        basicSwitchMasterPageTable(masterPageTableAddress);
    }
}

void PagingContext::completelyUnmapLoaderRegion() {
    /* this may be different on future supported platforms */
    static_assert(LoaderRegion.start == UserSpaceRegion.start
                  && LoaderRegion.length == UserSpaceRegion.length);

    for (size_t index = 0; index < KERNEL_ENTRIES_OFFSET; index++) {
        PageTableEntry &entry = masterPageTable->entries[index];
        if (entry.present()) {
            entry.addressValue().identityMapped().asPointer<PageTable>()->unmapEverything(MASTER_LEVEL - 1);
            CurrentSystem.memory.freePhysicalFrame(entry.addressValue());
        }
    }
}
