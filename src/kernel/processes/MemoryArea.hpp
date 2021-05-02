#pragma once

#include <common/common.hpp>
#include <memory/FrameManagement.hpp>
#include <processes/Frame.hpp>
#include <vector>

class PagingContext;

class MemoryArea {
private:
    vector<Frame *> frames;
    vector<FutexFrameID> futexIDs;
    /* should be held when changing anything in the frames vector */
    mutex lock;

public:
    using Source = hos_v1::MappingSource;
    const bool isShared;
    const Source source;
    const Permissions permissions;
    const size_t length;

    /* Anonymous */
    MemoryArea(bool shared, Permissions permissions, size_t length, Status &status);
    /* DMA */
    MemoryArea(frameManagement::ZoneID zoneID,
               size_t length,
               vector<size_t> &deviceAddresses,
               Status &status);
    /* Physical */
    MemoryArea(Permissions permissions, PhysicalAddress physicalStart, size_t length, Status &status);
    /* HandOver */
    MemoryArea(const hos_v1::MemoryArea &handOver, Status &status);
    ~MemoryArea();

    Status pageIn(PagingContext &context, UserVirtualAddress address, size_t offset);
    Status pageOut(PagingContext &context, UserVirtualAddress address, size_t offset);
    bool allowesPermissions(Permissions toTest) const;
    bool isAnonymous() const;

    // new functions
    Result<uint32_t> atomicCompareExchange32(size_t offset,
                                             uint32_t expectedValue,
                                             uint32_t newValue);
    /* PhysicalAddress returned for Futex manager use only. May have changed already since
     * ProcessAddressSpace is unlocked! */
    Result<PhysicalAddress> resolve(size_t offset);
};
