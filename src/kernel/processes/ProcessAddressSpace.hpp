#pragma once

#include <mutex>
#include <vector>
#include <paging/PagingContext.hpp>
#include <processes/MappedAreaVector.hpp>
#include <utility>

class Thread;

class ProcessAddressSpace {
private:
    friend class Frame;
    mutex lock;

    PagingContext pagingContext;
    vector<TLBContextID> inTLBContextOfProcessor;
    vector<MappedArea *> mappedAreas;

    pair<size_t, bool> findIndexFor(size_t userAddress);
    optional<pair<shared_ptr<MemoryArea> &, Region>> findMemoryArea(size_t userAddress,
                                                                    bool requireWritePermissions);

public:
    ProcessAddressSpace(Status &status);
    ProcessAddressSpace(const hos_v1::Process &handOver,
                        const vector<shared_ptr<MemoryArea>> &allMemoryAreas,
                        Status &status);
    ProcessAddressSpace(ProcessAddressSpace &) = delete;
    ProcessAddressSpace operator=(ProcessAddressSpace &) = delete;
    ~ProcessAddressSpace();

    Status addAnonymous(Region region, Permissions permissions);
    Result<Region> addAnonymous(size_t length, Permissions permissions);
    /* removeRegion is the slower, more complicated way to remove. Unlike removeMapping, it
     * supports things like unmapping a MappedArea partially, or multiple of them. It is there
     * to fully support mmap, munmap, etc. for in user space. However it currently makes no
     * attempt at optimizing these cases. */
    Status removeRegion(Region region);
    void removeMapping(size_t startAddress);

    Status copyFrom(uint8_t *destination,
                    size_t address,
                    size_t length);
    Status copyTo(size_t address,
                  const uint8_t *source,
                  size_t length,
                  bool requireWritePermissions);
    Status copyFromOhter(size_t destinationAddress,
                         ProcessAddressSpace &sourceProcess,
                         size_t sourceSpace,
                         size_t length,
                         bool requireWriteAccessToDestination);

    Result<uint32_t> atomicCopyFrom32(size_t address);
    Result<bool> atomicCompareExchange32(size_t address,
                                             uint32_t expectedValue,
                                             uint32_t newValue);

    Status coreDump(Thread *crashedThread);
};
