#pragma once

#include <common/common.hpp>
#include <paging/PagingContext.hpp>
#include <vector>

struct MemoryArea {
private:
    vector<PhysicalAddress> frames;

public:
    using Source = hos_v1::MappingSource;
    const Source source;
    const Permissions permissions;
    const size_t length;

    MemoryArea(size_t length);
    MemoryArea(Permissions permissions, size_t length);
    MemoryArea(Permissions permissions, PhysicalAddress physicalStart, size_t length);
    MemoryArea(size_t frameStack, size_t length, vector<size_t> &deviceAddresses);
    MemoryArea(const hos_v1::MemoryArea &handOver);

    PhysicalAddress makePresent(size_t offset);
    CacheType cacheType() const;
    bool allowesPermissions(Permissions toTest) const;
    bool isAnonymous() const;
};

