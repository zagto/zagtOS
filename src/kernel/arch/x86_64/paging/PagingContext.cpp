#include <common/common.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <processes/Process.hpp>
/* for lock asserts only */
#include <memory/KernelPageAllocator.hpp>


Result<PageTableEntry *> PagingContext::walkEntries(VirtualAddress address,
                                                    MissingStrategy missingStrategy) {
    assert(!address.isInRegion(IdentityMapping));

    PageTable *table = masterPageTable;

    for (size_t level = MASTER_LEVEL; level > 0; level--) {
        PageTableEntry *entry = table->entryFor(address, level);

        if (!entry->present()) {
            switch (missingStrategy) {
            case MissingStrategy::NONE:
                cout << "Unexpected non-present page table entry." << endl;
                Panic();
            case MissingStrategy::CREATE: {
                Result<PhysicalAddress> addr = FrameManagement.get(frameManagement::DEFAULT_ZONE_ID);
                if (!addr) {
                    return addr.status();
                }

                *entry = PageTableEntry(*addr,
                                        Permissions::READ_WRITE_EXECUTE,
                                        true,
                                        CacheType::NORMAL_WRITE_BACK);
                break;
            }
            }
        }

        table = entry->addressValue().identityMapped().asPointer<PageTable>();
    }
    return table->entryFor(address, 0);
}


PagingContext::PagingContext(Status &status) {
    Result<PhysicalAddress> result = FrameManagement.get(frameManagement::DEFAULT_ZONE_ID);
    if (!result) {
        status = result.status();
        return;
    }

    masterPageTableAddress = *result;
    masterPageTable = masterPageTableAddress.identityMapped().asPointer<PageTable>();

    for (size_t index = KERNEL_ENTRIES_OFFSET;
         index < PageTable::NUM_ENTRIES - 1;
         index++) {
         masterPageTable->entries[index] = CurrentSystem.kernelOnlyPagingContext.masterPageTable->entries[index];
    }
}


PagingContext::PagingContext(PhysicalAddress masterPageTableAddress, Status &) :
        masterPageTableAddress{masterPageTableAddress},
        masterPageTable{masterPageTableAddress.identityMapped().asPointer<PageTable>()} {
}

PagingContext::~PagingContext() {
    completelyUnmapUserRegion();
}

void PagingContext::map(KernelVirtualAddress from,
                          PhysicalAddress to,
                          Permissions permissions,
                          CacheType cacheType) {
    assert(KernelPageAllocator.lock.isLocked());

    Result<PageTableEntry *>entry = CurrentSystem.kernelOnlyPagingContext.walkEntries(from,
                                                                                      MissingStrategy::CREATE);
    assert(!(*entry)->present());

    **entry = PageTableEntry(to, permissions, false, cacheType);
}

void PagingContext::_unmap(VirtualAddress address, bool freeFrame) {
    Result<PageTableEntry *>entry = walkEntries(address, MissingStrategy::NONE);
    /* Exceptions should only happen with CREATE MissingStategy */
    assert(static_cast<bool>(entry));
    assert((*entry)->present());

    if (freeFrame) {
        FrameManagement.put((*entry)->addressValue());
    }

    **entry = PageTableEntry();
}

void PagingContext::unmap(UserVirtualAddress address) {
    _unmap(address, false);
}


void PagingContext::unmapRange(KernelVirtualAddress address, size_t numPages, bool freeFrames) {
    assert(KernelPageAllocator.lock.isLocked());
    for (size_t index = 0; index < numPages; index++) {
        CurrentSystem.kernelOnlyPagingContext._unmap(address + index * PAGE_SIZE, freeFrames);
    }
}

Status PagingContext::map(UserVirtualAddress from,
                          PhysicalAddress to,
                          Permissions permissions,
                          CacheType cacheType) {
    Result<PageTableEntry *> entry = walkEntries(from, MissingStrategy::CREATE);
    if (!entry) {
        return entry.status();
    }
    assert(!(*entry)->present());

    **entry = PageTableEntry(to, permissions, true, cacheType);
    return Status::OK();
}

extern "C" void basicSwitchMasterPageTable(PhysicalAddress address);

void PagingContext::activate() {
    basicSwitchMasterPageTable(masterPageTableAddress);
}

void PagingContext::completelyUnmapUserRegion() {
    /* this may be different on future supported platforms */
    static_assert(LoaderRegion.start == UserSpaceRegion.start
                  && LoaderRegion.length == UserSpaceRegion.length);

    for (size_t index = 0; index < KERNEL_ENTRIES_OFFSET; index++) {
        PageTableEntry &entry = masterPageTable->entries[index];
        if (entry.present()) {
            entry.addressValue().identityMapped().asPointer<PageTable>()->unmapEverything(MASTER_LEVEL - 1);
           // TODO: this may be wrong: FrameManagement.put(entry.addressValue());
        }
    }
}

void PagingContext::completelyUnmapLoaderRegion() {
    completelyUnmapUserRegion();
}
