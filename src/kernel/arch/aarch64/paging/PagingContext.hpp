#pragma once
#include <paging/PageTable.hpp>

class PagingContext {
public:
    static const size_t NUM_LEVELS{3};
    static const size_t MASTER_LEVEL{NUM_LEVELS - 1};

private:
    enum class MissingStrategy {
        NONE, CREATE
    };

    /* physical and virtual address of the same thing */
    PhysicalAddress kernelMasterPageTableAddress;
    PageTable *kernelMasterPageTable;
    PhysicalAddress userMasterPageTableAddress;
    PageTable *userMasterPageTable;

    PageTableEntry *walkEntries(PageTable *masterPageTable,
                                VirtualAddress address,
                                MissingStrategy missingStrategy);

public:
    enum class AccessOperation {
        READ, WRITE, VERIFY_ONLY
    };

    PagingContext();
    PagingContext(const hos_v1::PagingContext &handOverPagingContext);
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
