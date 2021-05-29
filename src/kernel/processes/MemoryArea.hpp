#pragma once

#include <common/common.hpp>
#include <memory/FrameManagement.hpp>
#include <processes/Frame.hpp>
#include <vector>

class ProcessAddressSpace;

class MemoryArea {
private:
    using Source = hos_v1::MappingSource;

    vector<Frame *> frames;
    vector<FutexFrameID> futexIDs;
    /* should be held when changing anything in the frames or frameMeta vector */
    mutex lock;
    const Source source;
    const Permissions permissions;
    /* for PHYSICAL source only */
    const PhysicalAddress physicalStart{PhysicalAddress::NULL};

    /* The frames vector may contain null pointers for lazy initialization. This method ensures
     * a frame actually exists */
    Status ensureFramePresent(size_t frameIndex, bool requireNoCopyOnWrite);

protected:
    friend class Frame;
    Status _copyFrom(uint8_t *destination,
                     size_t offset,
                     size_t accessLength,
                     scoped_lock<mutex> &scopedLock);

public:
    const bool isShared;
    const size_t length;

    /* Anonymous */
    MemoryArea(bool shared, Permissions permissions, size_t length, Status &status);
    /* DMA */
    MemoryArea(frameManagement::ZoneID zoneID,
               size_t length,
               vector<size_t> &deviceAddresses,
               Status &status);
    /* Physical */
    MemoryArea(PhysicalAddress physicalStart, size_t length, Status &status);
    /* HandOver */
    MemoryArea(const hos_v1::MemoryArea &handOver, vector<Frame *> &allFrames, Status &status);
    /* Shallow Copy */
    MemoryArea(MemoryArea &other, size_t offset, size_t length, Status &status);
    MemoryArea(MemoryArea &) = delete;
    MemoryArea operator=(MemoryArea &) = delete;
    ~MemoryArea();

    Status pageIn(ProcessAddressSpace &addressSpace,
                  UserVirtualAddress address,
                  Permissions mappingPermissions,
                  size_t offset);
    PageOutContext pageOut(ProcessAddressSpace &addressSpace,
                           UserVirtualAddress address,
                           size_t offset);
    bool allowesPermissions(Permissions toTest) const;
    Result<uint64_t> getFutexID(size_t offset);

    /* data access */
    Status copyFrom(uint8_t *destination, size_t offset, size_t accessLength);
    Status copyTo(size_t offset, const uint8_t *source, size_t accessLength);
    Status copyFromOther(size_t destinationOffset,
                         MemoryArea &sourceArea,
                         size_t sourceOffset,
                         size_t accessLength);
    Result<uint32_t> atomicCopyFrom32(size_t offset);
    Result<bool> atomicCompareExchange32(size_t offset,
                                         uint32_t expectedValue,
                                         uint32_t newValue);
};
