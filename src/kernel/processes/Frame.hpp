#pragma once
#include <common/common.hpp>
#include <memory/FrameManagement.hpp>
#include <memory/PageOutContext.hpp>

class ProcessAddressSpace;
class MemoryArea;

class Frame {
private:
    PhysicalAddress address = PhysicalAddress::NULL;
    size_t copyOnWriteCount = 1;
    size_t referenceCount = 1;
    bool isForPhysicalAccess = false;

public:
    Frame(frameManagement::ZoneID zoneID, Status &status);
    Frame(PhysicalAddress address, bool isForPhysicalAccess, Status &);
    Frame(Frame &other) = delete;
    Frame operator=(Frame &other) = delete;
    ~Frame();

    Result<Frame *> ensureNoCopyOnWrite();
    void copyOnWriteDuplicate();
    bool isCopyOnWrite() const;
    Status pageIn(ProcessAddressSpace &addressSpace,
                  UserVirtualAddress virtualAddress,
                  Permissions mappingPermissions);
    PageOutContext pageOut(ProcessAddressSpace &addressSpace, UserVirtualAddress virtualAddress);
    PhysicalAddress physicalAddress() const;
    void decreaseMemoryAreaReference();
    void decreaseInvalidateRequestReference();

    /* data access */
    Status copyFrom(uint8_t *destination, size_t offset, size_t length);
    Status copyTo(size_t offset, const uint8_t *source, size_t length);
    Status copyFromMemoryArea(size_t frameOffset,
                              MemoryArea &memoryArea,
                              size_t areaOffset,
                              size_t length,
                              scoped_lock<mutex> &scopedLock);
    Result<uint32_t> atomicCopyFrom32(size_t offset);
    Result<bool> atomicCompareExchange32(size_t offset,
                                         uint32_t expectedValue,
                                         uint32_t newValue);
};
