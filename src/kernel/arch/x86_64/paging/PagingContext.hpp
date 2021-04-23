#pragma once
#include <paging/PageTable.hpp>
#include <lib/Status.hpp>

class Process;

class PagingContext {
public:
    static const size_t NUM_LEVELS{4};
    static const size_t MASTER_LEVEL{NUM_LEVELS - 1};

private:
    enum class MissingStrategy {
        NONE, RETURN_NULLPTR, CREATE
    };
    struct WalkData {
        PageTable *tables[NUM_LEVELS];

        WalkData(PageTable *mpt) {
            tables[MASTER_LEVEL] = mpt;
        }
    };

    Process *process;

    /* physical and virtual address of the same thing */
    PhysicalAddress masterPageTableAddress;
    PageTable *masterPageTable;

    /* The partial version of this method exposes the path taken by the page walk via the walkData
     * parameter, which can be used to skip parts of the page walk on subsequent calls */
    Result<PageTableEntry *> partialWalkEntries(VirtualAddress address,
                                                MissingStrategy missingStrategy,
                                                size_t startLevel,
                                                WalkData &walkData);
    Result<PageTableEntry *> walkEntries(VirtualAddress address, MissingStrategy missingStrategy);
    void _unmapRange(VirtualAddress address, size_t numPages, bool freeFrames);

public:
    enum class AccessOperation {
        READ, WRITE, VERIFY_ONLY
    };

    static const size_t KERNEL_ENTRIES_OFFSET = PageTable::NUM_ENTRIES / 2;
    static const size_t NUM_KERNEL_ENTRIES = PageTable::NUM_ENTRIES - KERNEL_ENTRIES_OFFSET;

    PagingContext(Process *process, Status &status);
    PagingContext(Process *process, PhysicalAddress masterPageTableAddress, Status &);

    static void map(KernelVirtualAddress from,
                    PhysicalAddress to,
                    Permissions permissions,
                    CacheType cacheType);
    static PhysicalAddress resolve(KernelVirtualAddress address);
    static void invalidateLocally(KernelVirtualAddress address);
    static void unmapRange(KernelVirtualAddress address, size_t numPages, bool freeFrames);

    Status map(UserVirtualAddress from,
               PhysicalAddress to,
               Permissions permissions,
               CacheType cacheType);
    PhysicalAddress resolve(UserVirtualAddress address);
    Status accessRange(UserVirtualAddress address,
                       size_t numPages,
                       size_t startOffset,
                       size_t endOffset,
                       uint8_t *buffer,
                       AccessOperation accOp,
                       Permissions newPagesPermissions);
    void unmapRange(UserVirtualAddress address, size_t numPages, bool freeFrames);
    void changeRangePermissions(UserVirtualAddress address,
                                size_t numPages,
                                Permissions newPermissions);
    bool isMapped(UserVirtualAddress address);
    void invalidateLocally(UserVirtualAddress address);

    bool isActive();
    void activate();

    void completelyUnmapLoaderRegion();
};
