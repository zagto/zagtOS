#ifndef MASTERPAGETABLE_HPP
#define MASTERPAGETABLE_HPP

#include <paging/PageTable.hpp>


class alignas(PAGE_SIZE) MasterPageTable : public PageTable {
private:
    enum class MissingStrategy {
        NONE, RETURN_NULLPTR, CREATE
    };
    PageTableEntry *walkEntries(VirtualAddress address, MissingStrategy missingStrategy);

public:
    static const usize KERNEL_ENTRIES_OFFSET = PageTable::NUM_ENTRIES / 2;
    static const usize NUM_KERNEL_ENTRIES = PageTable::NUM_ENTRIES - KERNEL_ENTRIES_OFFSET;

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
    void unmap(UserVirtualAddress address);
    bool isMapped(UserVirtualAddress address);
    void invalidateLocally(UserVirtualAddress address);

    bool isActive();
    void activate();

    void completelyUnmapRegion(Region region);
};

#endif // MASTERPAGETABLE_HPP
