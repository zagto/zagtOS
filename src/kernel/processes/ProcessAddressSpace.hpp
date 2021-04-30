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

    Status copyFromUser(uint8_t *destination, size_t address, size_t length, bool requireWritePermissions);
    Status copyToUser(size_t address, const uint8_t *source, size_t length, bool requireWritePermissions);
    Status copyFromOhterUserSpace(size_t destinationAddress,
                                  Process &sourceProcess,
                                  size_t sourceAddress,
                                  size_t length,
                                bool requireWriteAccessToDestination);
    Status atomicCopyFrom32(size_t address);

    Status coreDump(Thread *crashedThread);
};
