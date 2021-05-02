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

    Result<shared_ptr<MemoryArea> &> findMemoryArea(UserVirtualAddress virtualAddress);

public:
    Status addAnonymous(Region region, Permissions permissions);
    Result<Region> addAnonymous(size_t length, Permissions permissions);
    void remove(Region region);

    Status copyFrom(uint8_t *destination,
                    size_t address,
                    size_t length,
                    bool requireWritePermissions);
    Status copyTo(size_t address,
                  const uint8_t *source,
                  size_t length,
                  bool requireWritePermissions);
    Status copyFromOhter(size_t destinationAddress,
                         ProcessAddressSpace &sourceProcess,
                         size_t sourceAddress,
                         size_t length,
                         bool requireWriteAccessToDestination);

    Result<uint32_t> atomicCopyFrom32(size_t address);
    Result<uint32_t> atomicCompareExchange32(size_t address,
                                             uint32_t expectedValue,
                                             uint32_t newValue);
    Result<PhysicalAddress> resolve(UserVirtualAddress virtualAddress);

    Status coreDump(Thread *crashedThread);
};
