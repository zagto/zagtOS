#ifndef MAPPEDAREA_HPP
#define MAPPEDAREA_HPP

#include <common/common.hpp>
#include <common/Region.hpp>
#include <lib/SortedVector.hpp>

class Task;

class MappedArea {
private:
    enum class Source {
        MEMORY = 1, PHYSICAL_MEMORY = 2,
    };
    Task *task;
    Source source;
    PhysicalAddress physicalStart;

public:
    Region region;
    Permissions permissions;

    MappedArea(Task *task, Region region, Permissions permissions);
    MappedArea(Task *task, Region region, Permissions permissions, PhysicalAddress physicalStart);
    ~MappedArea();

    bool handlePageFault(UserVirtualAddress address);
    void unmapRange(Region range);
    void shrinkFront(size_t amount);
    void shrinkBack(size_t amount);

    static inline bool compare(MappedArea *a, MappedArea *b) {
        return a->region.start < b->region.start;
    }
};

class MappedAreaVector : public SortedVector<MappedArea *, MappedArea::compare> {
private:
    Task *task;

    bool findMappedAreaIndexOrFreeLength(UserVirtualAddress address,
                                         size_t &resultIndex,
                                         size_t &freeLength);

public:
    MappedAreaVector(Task *task):
        task{task} {}
    Region findFreeRegion(size_t length, bool &valid, size_t &index);
    MappedArea *findMappedArea(UserVirtualAddress address);
    void insert2(MappedArea *ma, size_t index);
    MappedArea *addNew(size_t length, Permissions permissions);
    bool isRegionFree(Region region, size_t &insertIndex);
    void splitElement(size_t index, Region removeRegion, size_t numAddBetween);
    size_t unmapRange(Region range, size_t numAddInstead = 0);
};

#endif // MAPPEDAREA_HPP
