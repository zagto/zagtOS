#pragma once

#include <mutex>
#include <vector>
#include <paging/PagingContext.hpp>
#include <processes/MappedArea.hpp>
#include <utility>

class Thread;

class ProcessAddressSpace {
private:
    friend class Frame;
    mutex lock;

    PagingContext pagingContext;
    SpinLock tlbIDsLock;
    vector<TLBContextID> inTLBContextOfProcessor;
    vector<unique_ptr<MappedArea>> mappedAreas;

    pair<size_t, bool> findIndexFor(size_t userAddress) noexcept;

    optional<pair<shared_ptr<MemoryArea>, Region>>
    findMemoryArea(size_t userAddress, bool requireWritePermissions) noexcept;

    void _removeRegion(Region region);
    pair<Region, size_t> findFreeRegion(size_t length) const;
    void ensureSplitAt(size_t address);
    void ensureRegionIndependent(Region region);

    void copyFromLocked(uint8_t *destination, size_t address, size_t length);

public:
    ProcessAddressSpace();
    ProcessAddressSpace(const hos_v1::Process &handOver,
                        const vector<shared_ptr<MemoryArea>> &allMemoryAreas);
    ProcessAddressSpace(ProcessAddressSpace &) = delete;
    ProcessAddressSpace operator=(ProcessAddressSpace &) = delete;
    ~ProcessAddressSpace();

    bool operator==(const ProcessAddressSpace &other) const noexcept;
    bool operator!=(const ProcessAddressSpace &other) const noexcept;
    void activate();
    UserVirtualAddress add(Region region,
                           size_t offset,
                           shared_ptr<MemoryArea> memoryArea,
                           Permissions permissions,
                           bool overwriteExisiting);
    UserVirtualAddress addAnonymous(Region region,
                                    Permissions permissions,
                                    bool overwriteExisiting,
                                    bool shared);
    UserVirtualAddress addAnonymous(size_t length, Permissions permissions);
    /* removeRegion is the slower, more complicated way to remove. Unlike removeMapping, it
     * supports things like unmapping a MappedArea partially, or multiple of them. It is there
     * to fully support mmap, munmap, etc. for in user space. However it currently makes no
     * attempt at optimizing these cases. */
    void removeRegion(Region region);
    /* can return ENOMEM or EACCESS for MProtect syscall */
    size_t changeRegionProtection(Region region, Permissions permissions);
    void removeMapping(size_t startAddress);
    void handlePageFault(size_t address, Permissions requiredPermissions);
    uint64_t getFutexID(size_t address);
    void copyFrom(uint8_t *destination, size_t address, size_t length);
    void copyTo(size_t address,
                const uint8_t *source,
                size_t length,
                bool requireWritePermissions);
    void copyFromOhter(size_t destinationAddress,
                       ProcessAddressSpace &sourceProcess,
                       size_t sourceSpace,
                       size_t length,
                       bool requireWriteAccessToDestination);
    uint32_t atomicCopyFrom32(size_t address);
    bool atomicCompareExchange32(size_t address,
                                 uint32_t expectedValue,
                                 uint32_t newValue);

    shared_ptr<Process> process() const noexcept;

    void coreDump(Thread *crashedThread) ;
};
