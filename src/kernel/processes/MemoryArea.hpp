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
    const PhysicalAddress physicalStart{PhysicalAddress::Null};

    /* The frames vector may contain null pointers for lazy initialization. This method ensures
     * a frame actually exists */
    void ensureFramePresent(size_t frameIndex, bool requireNoCopyOnWrite);

    /* for destruction-like scenarios */
    void discardFrames() noexcept;

protected:
    friend class Frame;
    void _copyFrom(uint8_t *destination,
                   size_t offset,
                   size_t accessLength,
                   optional<scoped_lock<mutex> *> relaxLock);

public:
    const bool isShared;
    const size_t length;

    /* Anonymous */
    MemoryArea(bool shared, Permissions permissions, size_t length);
    /* DMA */
    MemoryArea(frameManagement::ZoneID zoneID,
               size_t length,
               vector<size_t> &deviceAddresses);
    /* Physical */
    MemoryArea(PhysicalAddress physicalStart, size_t length);
    /* HandOver */
    MemoryArea(const hos_v1::MemoryArea &handOver, vector<Frame *> &allFrames);
    /* Shallow Copy */
    MemoryArea(MemoryArea &other, size_t offset, size_t length);
    MemoryArea(MemoryArea &) = delete;
    MemoryArea operator=(MemoryArea &) = delete;
    ~MemoryArea();

    void pageIn(ProcessAddressSpace &addressSpace,
                UserVirtualAddress address,
                Permissions mappingPermissions,
                size_t offset);
    PageOutContext pageOut(ProcessAddressSpace &addressSpace,
                           UserVirtualAddress address,
                           size_t offset) noexcept;
    bool allowesPermissions(Permissions toTest) const noexcept;
    uint64_t getFutexID(size_t offset);

    /* data access */
    void copyFrom(uint8_t *destination, size_t offset, size_t accessLength);
    void copyTo(size_t offset, const uint8_t *source, size_t accessLength);
    void copyFromOther(size_t destinationOffset,
                       MemoryArea &sourceArea,
                       size_t sourceOffset,
                       size_t accessLength);
    uint32_t atomicCopyFrom32(size_t offset);
    bool atomicCompareExchange32(size_t offset,
                                 uint32_t expectedValue,
                                 uint32_t newValue);
};
