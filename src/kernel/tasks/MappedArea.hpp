#ifndef MAPPEDAREA_HPP
#define MAPPEDAREA_HPP

#include <common/common.hpp>
#include <common/Region.hpp>

class Task;

class MappedArea {
    enum class Source {
        MEMORY = 1, STACK = 2,
    };
    Task *task;
    Source source;
    Permissions permissions;

public:
    Region region;

    MappedArea(Task *task, Region region, Permissions permissions);

    bool handlePageFault(UserVirtualAddress address);
    void mapEverything();
    //void changePermissions(Permissions permissions);
};

#endif // MAPPEDAREA_HPP
