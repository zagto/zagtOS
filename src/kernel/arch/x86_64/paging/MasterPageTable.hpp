#ifndef MASTERPAGETABLE_HPP
#define MASTERPAGETABLE_HPP

#include <paging/PageTable.hpp>


class alignas(PAGE_SIZE) MasterPageTable : public PageTable {
private:
    enum class MissingStrategy {
        NONE, RETURN_NULLPTR, CREATE
    };
    struct WalkData {
        PageTable *tables[NUM_LEVELS];

        WalkData(MasterPageTable *mpt) {
            tables[MASTER_LEVEL] = mpt;
        }
    };

    /* The partial version of this method exposes the path taken by the page walk via the walkData
     * parameter, which can be used to skip parts of the page walk on subsequent calls */
    PageTableEntry *partialWalkEntries(VirtualAddress address,
                                       MissingStrategy missingStrategy,
                                       size_t startLevel,
                                       WalkData &walkData);
    PageTableEntry *walkEntries(VirtualAddress address, MissingStrategy missingStrategy);

public:
    enum class AccessOperation {
        READ, WRITE, VERIFY_ONLY, UNMAP, UNMAP_AND_FREE
    };

    static const size_t KERNEL_ENTRIES_OFFSET = PageTable::NUM_ENTRIES / 2;
    static const size_t NUM_KERNEL_ENTRIES = PageTable::NUM_ENTRIES - KERNEL_ENTRIES_OFFSET;

    MasterPageTable();

    static void map(KernelVirtualAddress from,
                    PhysicalAddress to,
                    Permissions permissions);
    PhysicalAddress resolve(KernelVirtualAddress address);
    static void unmap(KernelVirtualAddress address);
    static void invalidateLocally(KernelVirtualAddress address);

    void map(UserVirtualAddress from,
             PhysicalAddress to,
             Permissions permissions);
    PhysicalAddress resolve(UserVirtualAddress address);
    void accessRange(UserVirtualAddress address,
                     size_t numPages,
                     size_t startOffset,
                     size_t endOffset,
                     uint8_t *buffer,
                     AccessOperation accOp,
                     Permissions newPagesPermissions);
    void unmap(UserVirtualAddress address);
    bool isMapped(UserVirtualAddress address);
    void invalidateLocally(UserVirtualAddress address);

    bool isActive();
    void activate();

    void completelyUnmapLoaderRegion();
};

#endif // MASTERPAGETABLE_HPP
