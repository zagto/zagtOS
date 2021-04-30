#pragma once

#include <mutex>
#include <vector>
#include <paging/PagingContext.hpp>
#include <processes/MappedAreaVector.hpp>

class Thread;

class ProcessAddressSpace {
private:
    mutex lock;

    PagingContext pagingContext;
    vector<uint64_t> inTLBContextSince;
    MappedAreaVector mappedAreas;

public:
    Status addAnonymous(Region region, Permissions permissions);
    Result<Region> addAnonymous(size_t length, Permissions permissions);
    void remove(Region region);
    Status coreDump(Thread *crashedThread);
};
