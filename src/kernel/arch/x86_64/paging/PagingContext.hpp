#pragma once
#include <paging/PageTable.hpp>

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

    PageTableEntry *walkEntries(VirtualAddress address, MissingStrategy missingStrategy);
    void _unmap(VirtualAddress address, bool freeFrame) noexcept;

public:
    enum class AccessOperation {
        READ, WRITE, VERIFY_ONLY
    };

    static const size_t KERNEL_ENTRIES_OFFSET = PageTable::NUM_ENTRIES / 2;
    static const size_t NUM_KERNEL_ENTRIES = PageTable::NUM_ENTRIES - KERNEL_ENTRIES_OFFSET;

    PagingContext();
    PagingContext(const hos_v1::PagingContext &handOver);
    PagingContext(PagingContext &other) = delete;
    PagingContext operator=(PagingContext &other) = delete;
    ~PagingContext() noexcept;

    static void map(KernelVirtualAddress from,
                    PhysicalAddress to,
                    Permissions permissions,
                    CacheType cacheType);
    static void unmap(KernelVirtualAddress address, bool freeFrame) noexcept;
    static void unmapRange(KernelVirtualAddress address, size_t numPages, bool freeFrames) noexcept;

    void map(UserVirtualAddress from,
             PhysicalAddress to,
             Permissions permissions,
             CacheType cacheType);
    void unmap(UserVirtualAddress address) noexcept;

    void activate() noexcept;

    void completelyUnmapLoaderRegion() noexcept;
    void completelyUnmapUserRegion() noexcept;
};
