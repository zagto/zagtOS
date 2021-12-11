#pragma once

#include <common/common.hpp>
#include <common/Region.hpp>
#include <processes/MemoryArea.hpp>
#include <utility>
#include <memory>

class MappedArea {
private:
    ProcessAddressSpace &addressSpace;
    vector<bool> isPagedIn;

    PageOutContext pageOutRegion(Region removeRegion);

public:
    shared_ptr<MemoryArea> memoryArea;
    size_t offset;
    Region region;
    Permissions permissions;

    MappedArea(ProcessAddressSpace &addressSpace,
               Region region,
               shared_ptr<MemoryArea> _memoryArea,
               size_t offset,
               Permissions permissions) ;
    MappedArea(ProcessAddressSpace &addressSpace,
               shared_ptr<MemoryArea> _memoryArea,
               const hos_v1::MappedArea &handOver);
    ~MappedArea();
    hos_v1::MappedArea handOver() noexcept;

    void ensurePagedIn(UserVirtualAddress address);
    pair<unique_ptr<MappedArea>, unique_ptr<MappedArea>> split(size_t splitOffset);

    static inline bool compare(MappedArea *a, MappedArea *b) noexcept {
        return a->region.start < b->region.start;
    }
};
