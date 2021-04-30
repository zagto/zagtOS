#pragma once

#include <common/common.hpp>
#include <common/Region.hpp>
#include <processes/MemoryArea.hpp>

class Process;

class MappedArea {
private:
    Process *process;

public:
    shared_ptr<MemoryArea> memoryArea;
    size_t offset;
    Region region;
    Permissions permissions;

    MappedArea(Process *process,
               Region region,
               shared_ptr<MemoryArea> _memoryArea,
               size_t offset,
               Permissions permissions,
               Status &status);
    MappedArea(Process *process,
               shared_ptr<MemoryArea> _memoryArea,
               const hos_v1::MappedArea &handOver,
               Status &status);
    ~MappedArea();

    Status handlePageFault(UserVirtualAddress address);
    void unmapRange(Region range);
    void shrinkFront(size_t amount);
    void shrinkBack(size_t amount);
    void changePermissions(Permissions newPermissions);

    static inline bool compare(MappedArea *a, MappedArea *b) {
        return a->region.start < b->region.start;
    }
};
