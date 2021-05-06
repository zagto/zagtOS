#pragma once

#include <common/common.hpp>
#include <vector>

class MappedArea;
class MemoryArea;

class MappedAreaVector : public vector<MappedArea *> {
private:
    bool findMappedAreaIndexOrFreeLength(UserVirtualAddress address,
                                         size_t &resultIndex,
                                         size_t &freeLength) const;

public:
    MappedAreaVector() : vector<MappedArea *>() {}
    MappedAreaVector(const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
                     const hos_v1::Process &handOver,
                     Status &status);
    Region findFreeRegion(size_t length, bool &valid, size_t &index) const;
    optional<MappedArea *> findMappedArea(UserVirtualAddress address, size_t &nextIndex) const;
    optional<MappedArea *> findMappedArea(UserVirtualAddress address) const;
    bool isRegionFree(Region region, size_t &insertIndex) const;
    void splitElement(size_t index, Region removeRegion, size_t numAddBetween);
    Result<size_t> unmapRange(Region range, size_t numAddInstead = 0);
    bool isRegionFullyMapped(Region range, size_t &index) const;
    bool changeRangePermissions(Region range, Permissions newPermissions);
};
