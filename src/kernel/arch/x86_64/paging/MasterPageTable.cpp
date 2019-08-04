#include <common/common.hpp>
#include <system/System.hpp>
#include <paging/MasterPageTable.hpp>
#include <system/System.hpp>
#include <tasks/Task.hpp>



extern "C" void basicInvalidate(VirtualAddress address);


PageTableEntry *MasterPageTable::partialWalkEntries(VirtualAddress address,
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
                return nullptr;
            case MissingStrategy::CREATE: {
                PhysicalAddress frame = CurrentSystem.memory.allocatePhysicalFrame();
                *entry = PageTableEntry(frame, Permissions::WRITE_AND_EXECUTE, true);
                break;
            }
            }
        }

        data.tables[level-1] = entry->addressValue().identityMapped().asPointer<PageTable>();
    }
    return data.tables[0]->entryFor(address, 0);
}


PageTableEntry *MasterPageTable::walkEntries(VirtualAddress address, MissingStrategy ms) {
    WalkData walkData(this);
    return partialWalkEntries(address, ms, MASTER_LEVEL, walkData);
}


MasterPageTable::MasterPageTable() {
    assert(this != CurrentProcessor->activeMasterPageTable);

    for (size_t index = KERNEL_ENTRIES_OFFSET;
         index < NUM_ENTRIES - 1;
         index++) {
         entries[index] = CurrentProcessor->activeMasterPageTable->entries[index];
    }
}


void MasterPageTable::map(KernelVirtualAddress from,
                          PhysicalAddress to,
                          Permissions permissions) {
    PageTableEntry *entry = CurrentSystem.kernelOnlyMasterPageTable->walkEntries(from, MissingStrategy::CREATE);
    assert(!entry->present());

    *entry = PageTableEntry(to, permissions, false);
    basicInvalidate(from);
}


PhysicalAddress MasterPageTable::resolve(KernelVirtualAddress address) {
    return CurrentSystem.kernelOnlyMasterPageTable->walkEntries(address,
                                                                MissingStrategy::NONE)->addressValue();
}


void MasterPageTable::unmap(KernelVirtualAddress address) {
    PageTableEntry *entry = CurrentSystem.kernelOnlyMasterPageTable->walkEntries(address, MissingStrategy::NONE);
    *entry = PageTableEntry();
}


PhysicalAddress MasterPageTable::resolve(UserVirtualAddress address) {
    return walkEntries(address, MissingStrategy::NONE)->addressValue();
}


void MasterPageTable::accessRange(UserVirtualAddress address,
                                  size_t numPages,
                                  size_t startOffset,
                                  size_t endOffset,
                                  uint8_t *buffer,
                                  AccessOperation accOp,
                                  Permissions newPagesPermissions) {
    assert(address.isPageAligned());
    assert(startOffset < PAGE_SIZE);
    assert(endOffset < PAGE_SIZE);
    assert(numPages > 1 || startOffset + endOffset < PAGE_SIZE);

    if (accOp == AccessOperation::UNMAP || accOp == AccessOperation::UNMAP_AND_FREE) {
        assert(startOffset == 0);
        assert(endOffset == 0);
        assert(buffer == nullptr);
    }

    WalkData walkData(this);
    size_t indexes[NUM_LEVELS];

    for (size_t i = 0; i < NUM_LEVELS; i++) {
        indexes[i] = indexFor(address, i);
    }

    /* TODO: in case of UNMAP operation, check for entries before the index, and unmap the table
     * itself if there is none. Currently we may be leaking memory */

    size_t changedLevel = MASTER_LEVEL;

    while (true) {
        PageTableEntry *entry = partialWalkEntries(address,
                                                   MissingStrategy::CREATE,
                                                   changedLevel,
                                                   walkData);
        if (!entry->present() && accOp != AccessOperation::UNMAP) {
            PhysicalAddress frame = CurrentSystem.memory.allocatePhysicalFrame();
            *entry = PageTableEntry(frame, newPagesPermissions, true);
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
        case AccessOperation::UNMAP:
            if (entry->present()) {
                *entry = PageTableEntry();
            }
            break;
        case AccessOperation::UNMAP_AND_FREE:
            if (entry->present()) {
                CurrentSystem.memory.freePhysicalFrame(entry->addressValue());
                *entry = PageTableEntry();
            }
            break;
        default:
            cout << "accessRange: invalid operation\n";
            Panic();
        }

        /* set startOffset to zero so it is only applied on first page */
        startOffset = 0;
        buffer += lengthInPage;

        changedLevel = 0;
        while (indexes[changedLevel] == NUM_ENTRIES) {
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


void MasterPageTable::map(UserVirtualAddress from,
                          PhysicalAddress to,
                          Permissions permissions) {
    PageTableEntry *entry = walkEntries(from, MissingStrategy::CREATE);
    assert(!entry->present());

    *entry = PageTableEntry(to, permissions, true);
    if (isActive()) {
        basicInvalidate(from);
    }
}


void MasterPageTable::unmap(UserVirtualAddress address) {
    PageTableEntry *entry = walkEntries(address, MissingStrategy::NONE);
    *entry = PageTableEntry();
}


bool MasterPageTable::isMapped(UserVirtualAddress address) {
    PageTableEntry *entry = walkEntries(address, MissingStrategy::RETURN_NULLPTR);
    return entry != nullptr && entry->present();
}


void MasterPageTable::invalidateLocally(KernelVirtualAddress address) {
    basicInvalidate(address);
}

void MasterPageTable::invalidateLocally(UserVirtualAddress address) {
    assert(isActive());
    assert(this == CurrentProcessor->currentTask->masterPageTable);
    assert(CurrentProcessor->currentTask->pagingLock.isLocked());

    basicInvalidate(address);
}

bool MasterPageTable::isActive() {
    return CurrentProcessor->activeMasterPageTable == this;
}

extern "C" void basicSwitchMasterPageTable(PhysicalAddress address);

void MasterPageTable::activate() {
    if (!isActive()) {
        CurrentProcessor->activeMasterPageTable = this;
        cout << (size_t)this << "\n";
        basicSwitchMasterPageTable(MasterPageTable::resolve(KernelVirtualAddress(this)));
    }
}

void MasterPageTable::completelyUnmapLoaderRegion() {
    /* this may be different on future supported platforms */
    static_assert(LoaderRegion.start == UserSpaceRegion.start
                  && LoaderRegion.length == UserSpaceRegion.length);

    for (size_t index = 0; index < KERNEL_ENTRIES_OFFSET; index++) {
        PageTableEntry &entry = entries[index];
        if (entry.present()) {
            entry.addressValue().identityMapped().asPointer<PageTable>()->unmapEverything(MASTER_LEVEL - 1);
            CurrentSystem.memory.freePhysicalFrame(entry.addressValue());
        }
    }
}
