#pragma once
#include <common/common.hpp>
#include <memory/FrameManagement.hpp>
#include <memory/PageOutContext.hpp>

class ProcessAddressSpace;
class MemoryArea;

class Frame {
private:
    PhysicalAddress address = PhysicalAddress::Null;
    size_t copyOnWriteCount = 1;
    size_t referenceCount = 1;
    bool isForPhysicalAccess = false;

public:
    Frame(frameManagement::ZoneID zoneID);
    Frame(PhysicalAddress address, bool isForPhysicalAccess) noexcept;
    Frame(hos_v1::Frame &handOver) noexcept;
    Frame(Frame &other) = delete;
    Frame operator=(Frame &other) = delete;
    ~Frame();

    Frame *ensureNoCopyOnWrite();
    void copyOnWriteDuplicate() noexcept;
    bool isCopyOnWrite() const noexcept;
    void pageIn(ProcessAddressSpace &addressSpace,
                UserVirtualAddress virtualAddress,
                Permissions mappingPermissions);
    PageOutContext pageOut(ProcessAddressSpace &addressSpace,
                           UserVirtualAddress virtualAddress) noexcept;
    PhysicalAddress physicalAddress() const noexcept;
    void decreaseMemoryAreaReference() noexcept;
    void decreaseInvalidateRequestReference() noexcept;

    /* data access */
    void copyFrom(uint8_t *destination, size_t offset, size_t length) noexcept;
    void copyTo(size_t offset, const uint8_t *source, size_t length) noexcept;
    void copyFromMemoryArea(size_t frameOffset,
                            MemoryArea &memoryArea,
                            size_t areaOffset,
                            size_t length);
    uint32_t atomicCopyFrom32(size_t offset) noexcept;
    bool atomicCompareExchange32(size_t offset,
                                 uint32_t expectedValue,
                                 uint32_t newValue) noexcept;
};
