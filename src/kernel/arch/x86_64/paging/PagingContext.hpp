#pragma once
#include <paging/PageTable.hpp>
#include <lib/Status.hpp>

class PagingContext {
public:
    static const size_t NUM_LEVELS{4};
    static const size_t MASTER_LEVEL{NUM_LEVELS - 1};

private:
    enum class MissingStrategy {
        NONE, CREATE
    };

    /* physical and virtual address of the same thing */
    PhysicalAddress masterPageTableAddress;
    PageTable *masterPageTable;

    Result<PageTableEntry *> walkEntries(VirtualAddress address, MissingStrategy missingStrategy);
    void _unmap(VirtualAddress address, bool freeFrame);

public:
    enum class AccessOperation {
        READ, WRITE, VERIFY_ONLY
    };

    static const size_t KERNEL_ENTRIES_OFFSET = PageTable::NUM_ENTRIES / 2;
    static const size_t NUM_KERNEL_ENTRIES = PageTable::NUM_ENTRIES - KERNEL_ENTRIES_OFFSET;

    PagingContext(Status &status);
    PagingContext(PhysicalAddress masterPageTableAddress, Status &);
    PagingContext(PagingContext &other) = delete;
    PagingContext operator=(PagingContext &other) = delete;
    ~PagingContext();

    static void map(KernelVirtualAddress from,
                    PhysicalAddress to,
                    Permissions permissions,
                    CacheType cacheType);
    static void invalidateLocally(KernelVirtualAddress address);
    static void unmapRange(KernelVirtualAddress address, size_t numPages, bool freeFrames);

    Status map(UserVirtualAddress from,
               PhysicalAddress to,
               Permissions permissions,
               CacheType cacheType);
    Result<PhysicalAddress> resolve(UserVirtualAddress address);
    void unmap(UserVirtualAddress address);

    bool isActive();
    void activate();

    void completelyUnmapLoaderRegion();
};
