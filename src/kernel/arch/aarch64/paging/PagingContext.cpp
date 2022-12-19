#include <common/common.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <processes/Process.hpp>
/* for lock asserts only */
#include <memory/KernelPageAllocator.hpp>


PageTableEntry *PagingContext::walkEntries(PageTable *masterPageTable,
                                           VirtualAddress address,
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
                PhysicalAddress addr = FrameManagement.get(frameManagement::DEFAULT_ZONE_ID);

                *entry = PageTableEntry(addr,
                                        Permissions::READ_WRITE_EXECUTE,
                                        false,
                                        true,
                                        CacheType::NORMAL_WRITE_BACK);
                asm volatile("dsb ish");
                break;
            }
            }
        }

        table = entry->addressValue().identityMapped().asPointer<PageTable>();
    }
    return table->entryFor(address, 0);
}


PagingContext::PagingContext() {
    userMasterPageTableAddress = FrameManagement.get(frameManagement::DEFAULT_ZONE_ID);
    userMasterPageTable = userMasterPageTableAddress.identityMapped().asPointer<PageTable>();
    kernelMasterPageTableAddress =
            CurrentSystem.kernelOnlyPagingContext.kernelMasterPageTableAddress;
    kernelMasterPageTable = CurrentSystem.kernelOnlyPagingContext.kernelMasterPageTable;
}


PagingContext::PagingContext(const hos_v1::PagingContext &handOver) :
        kernelMasterPageTableAddress{handOver.highRoot},
        kernelMasterPageTable{kernelMasterPageTableAddress.identityMapped().asPointer<PageTable>()},
        userMasterPageTableAddress{handOver.lowRoot},
        userMasterPageTable{userMasterPageTableAddress.identityMapped().asPointer<PageTable>()} {
    cout << "PagingContext::PagingContext()" << endl;
}

PagingContext::~PagingContext() noexcept {
    completelyUnmapUserRegion();
}

void PagingContext::map(KernelVirtualAddress from,
                        PhysicalAddress to,
                        Permissions permissions,
                        CacheType cacheType) {
    assert(KernelPageAllocator.lock.isLocked());

    PageTableEntry *entry = CurrentSystem.kernelOnlyPagingContext.walkEntries(
                CurrentSystem.kernelOnlyPagingContext.kernelMasterPageTable,
                from,
                MissingStrategy::CREATE);

    assert(!entry->present());

    *entry = PageTableEntry(to, permissions, true, false, cacheType);
    asm volatile("dsb ish");
}

void PagingContext::unmap(UserVirtualAddress address) noexcept {
    asm volatile("dsb ish");

    /* Exceptions should only happen with CREATE MissingStategy */
    PageTableEntry *entry = walkEntries(userMasterPageTable,
                                        address,
                                        MissingStrategy::NONE);
    assert(entry->present());
    *entry = PageTableEntry();
}


void PagingContext::unmap(KernelVirtualAddress address, bool freeFrame) noexcept {
    assert(KernelPageAllocator.lock.isLocked());
    asm volatile("dsb ish");

    /* Exceptions should only happen with CREATE MissingStategy */
    PageTableEntry *entry = CurrentSystem.kernelOnlyPagingContext.walkEntries(
                CurrentSystem.kernelOnlyPagingContext.kernelMasterPageTable,
                address,
                MissingStrategy::NONE);
    assert(entry->present());

    if (freeFrame) {
        FrameManagement.put(entry->addressValue());
    }

    *entry = PageTableEntry();
}

void PagingContext::unmapRange(KernelVirtualAddress address,
                               size_t numPages,
                               bool freeFrames) noexcept {
    assert(KernelPageAllocator.lock.isLocked());
    for (size_t index = 0; index < numPages; index++) {
        PagingContext::unmap(address + index * PAGE_SIZE, freeFrames);
    }
}

void PagingContext::map(UserVirtualAddress from,
                        PhysicalAddress to,
                        Permissions permissions,
                        CacheType cacheType) {
    PageTableEntry *entry = walkEntries(userMasterPageTable,
                                        from,
                                        MissingStrategy::CREATE);
    assert(!entry->present());

    *entry = PageTableEntry(to, permissions, true, true, cacheType);
    asm volatile("dsb ish");
}

extern "C" void basicSwitchMasterPageTable(PhysicalAddress address);

void PagingContext::activate() noexcept {
    basicSwitchMasterPageTable(userMasterPageTableAddress);
}

void PagingContext::completelyUnmapUserRegion() noexcept {
    /* this may be different on future supported platforms */
    static_assert(LoaderRegion.start == UserSpaceRegion.start
                  && LoaderRegion.length == UserSpaceRegion.length);

    for (size_t index = 0; index < PageTable::NUM_ENTRIES; index++) {
        PageTableEntry &entry = userMasterPageTable->entries[index];
        if (entry.present()) {
            entry.addressValue().identityMapped().asPointer<PageTable>()->unmapEverything(MASTER_LEVEL - 1);
           // TODO: this may be wrong: FrameManagement.put(entry.addressValue());
        }
    }
}

void PagingContext::completelyUnmapLoaderRegion() noexcept {
    completelyUnmapUserRegion();
}
