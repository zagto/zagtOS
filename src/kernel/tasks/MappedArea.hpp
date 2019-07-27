#ifndef MAPPEDAREA_HPP
#define MAPPEDAREA_HPP

#include <common/common.hpp>
#include <common/Region.hpp>
#include <lib/SortedVector.hpp>

class Task;

class MappedArea {
private:
    enum class Source {
        MEMORY = 1, STACK = 2,
    };
    Task *task;
    Source source;

public:
    Region region;
    Permissions permissions;

    MappedArea(Task *task, Region region, Permissions permissions);

    bool handlePageFault(UserVirtualAddress address);
    void mapEverything();

    static inline bool compare(MappedArea *a, MappedArea *b) {
        return a->region.start < b->region.start;
    }
};

class MappedAreaVector : public SortedVector<MappedArea *, MappedArea::compare> {
private:
    Task *task;

public:
    MappedAreaVector(Task *task):
        task{task} {}
    Region findFreeRegion(size_t length, bool &valid, size_t &index);
    MappedArea *findMappedArea(UserVirtualAddress address);
    MappedArea *addNew(size_t length, Permissions permissions);
};

#endif // MAPPEDAREA_HPP
