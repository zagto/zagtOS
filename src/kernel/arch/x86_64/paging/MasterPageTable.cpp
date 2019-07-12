#include <common/common.hpp>
#include <system/System.hpp>
#include <paging/MasterPageTable.hpp>
#include <system/System.hpp>
#include <tasks/Task.hpp>



extern "C" void basicInvalidate(VirtualAddress address);


PageTableEntry *MasterPageTable::walkEntries(VirtualAddress address, MissingStrategy ms) {
    Assert(!address.isInRegion(IdentityMapping));

    PageTable *table = this;

    for (usize level = NUM_LEVELS; level > 1; level--) {
        PageTableEntry *entry = table->entryFor(address, level);

        if (!entry->present()) {
            switch (ms) {
            case MissingStrategy::NONE:
                Log << "non-present entry during resolve which should not have happened\n";
                Panic();
            case MissingStrategy::RETURN_NULLPTR:
                return nullptr;
            case MissingStrategy::CREATE: {
                PhysicalAddress frame = CurrentSystem.memory.allocatePhysicalFrame();
                Log << "frame: " << frame.value() << "\n";

                *entry = PageTableEntry(frame, Permissions::WRITE_AND_EXECUTE, true);
                break;
            }
            }
        }

        table = entry->addressValue().identityMapped().asPointer<PageTable>();
    }
    return table->entryFor(address, 1);
}


MasterPageTable::MasterPageTable() {
    Assert(this != CurrentProcessor->activeMasterPageTable);

    for (usize index = KERNEL_ENTRIES_OFFSET;
         index < NUM_ENTRIES - 1;
         index++) {
         entries[index] = CurrentProcessor->activeMasterPageTable->entries[index];
    }
}


void MasterPageTable::map(KernelVirtualAddress from,
                          PhysicalAddress to,
                          Permissions permissions) {
    PageTableEntry *entry = CurrentSystem.kernelOnlyMasterPageTable->walkEntries(from, MissingStrategy::CREATE);
    Assert(!entry->present());

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


void MasterPageTable::map(UserVirtualAddress from,
                          PhysicalAddress to,
                          Permissions permissions) {
    PageTableEntry *entry = walkEntries(from, MissingStrategy::CREATE);
    Assert(!entry->present());

    *entry = PageTableEntry(to, permissions, false);
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
    Assert(isActive());
    Assert(this == CurrentProcessor->currentTask->masterPageTable);
    Assert(CurrentProcessor->currentTask->pagingLock.isLocked());

    basicInvalidate(address);
}

bool MasterPageTable::isActive() {
    return CurrentProcessor->activeMasterPageTable == this;
}

extern "C" void basicSwitchMasterPageTable(PhysicalAddress address);

void MasterPageTable::activate() {
    if (!isActive()) {
        CurrentProcessor->activeMasterPageTable = this;
        Log << (usize)this << "\n";
        basicSwitchMasterPageTable(MasterPageTable::resolve(KernelVirtualAddress(this)));
    }
}

void MasterPageTable::completelyUnmapRegion(Region region) {
    usize end = region.start + region.length;

    Assert(entryFor(end, NUM_LEVELS-1) != entryFor(end+1, NUM_LEVELS-1));

    /* TODO */
}
