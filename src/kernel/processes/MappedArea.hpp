#pragma once

#include <common/common.hpp>
#include <common/Region.hpp>
#include <lib/SortedVector.hpp>

class Process;

class MappedArea {
private:
    Process *process;
    PhysicalAddress physicalStart;

public:
    using Source = hos_v1::MappingSource;
    Source source;
    Region region;
    Permissions permissions;

    MappedArea(Process *process, Region region, Permissions permissions);
    MappedArea(Process *process, Region region, Permissions permissions, PhysicalAddress physicalStart);
    MappedArea(Process *process, Region region);
    MappedArea(Process *process, const hos_v1::MappedArea &handOver);
    ~MappedArea();

    bool handlePageFault(UserVirtualAddress address);
    void unmapRange(Region range);
    void shrinkFront(size_t amount);
    void shrinkBack(size_t amount);
    void changePermissions(Permissions newPermissions);

    static inline bool compare(MappedArea *a, MappedArea *b) {
        return a->region.start < b->region.start;
    }
};

class MappedAreaVector : public SortedVector<MappedArea *, MappedArea::compare> {
private:
    Process *process;

    bool findMappedAreaIndexOrFreeLength(UserVirtualAddress address,
                                         size_t &resultIndex,
                                         size_t &freeLength) const;

public:
    MappedAreaVector(Process *process):
        process{process} {}
    MappedAreaVector(Process *process, const hos_v1::Process &handOver);
    Region findFreeRegion(size_t length, bool &valid, size_t &index) const;
    MappedArea *findMappedArea(UserVirtualAddress address) const;
    void insert2(MappedArea *ma, size_t index);
    MappedArea *addNew(size_t length, Permissions permissions);
    bool isRegionFree(Region region, size_t &insertIndex) const;
    void splitElement(size_t index, Region removeRegion, size_t numAddBetween);
    size_t unmapRange(Region range, size_t numAddInstead = 0);
    bool isRegionFullyMapped(Region range, size_t &index) const;
    bool changeRangePermissions(Region range, Permissions newPermissions);
};
