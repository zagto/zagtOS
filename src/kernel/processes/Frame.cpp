#include <processes/Frame.hpp>
#include <processes/ProcessAddressSpace.hpp>
#include <processes/MemoryArea.hpp>
#include <processes/Process.hpp>
#include <memory/TLBContext.hpp>
#include <system/Processor.hpp>

/* TODO: figure out what memory ordering is actually needed instead of __ATOMIC_SEQ_CST
 * everywhere */

Frame::Frame(frameManagement::ZoneID zoneID) {
    address = FrameManagement.get(zoneID);
}

Frame::Frame(PhysicalAddress address, bool isForPhysicalAccess) noexcept :
    address{address},
    isForPhysicalAccess{isForPhysicalAccess} {}

Frame::Frame(hos_v1::Frame &handOver) noexcept:
    address{handOver.address},
    copyOnWriteCount{handOver.copyOnWriteCount},
    referenceCount{handOver.copyOnWriteCount},
    isForPhysicalAccess{handOver.isForPhysicalAccess} {}

Frame::~Frame() {
    if (address.value() != PhysicalAddress::Null && !isForPhysicalAccess) {
        FrameManagement.put(address);
    }
}

Frame *Frame::ensureNoCopyOnWrite() {
    size_t cowCount = __atomic_load_n(&copyOnWriteCount, __ATOMIC_SEQ_CST);

    if (cowCount == 1) {
        return this;
    }

    /* cowCount > 1, let's make our own copy of the frame */
    assert(!isForPhysicalAccess);

    /* Copy-on-write should never be used on DMA frames, so we can aussme default zone */
    PhysicalAddress newAddress = FrameManagement.get(frameManagement::DEFAULT_ZONE_ID);
    Frame *result;
    try {
       result = new Frame(newAddress, false);
    }  catch (...) {
        FrameManagement.put(newAddress);
        throw;
    }

    if (__atomic_sub_fetch(&copyOnWriteCount, 1, __ATOMIC_SEQ_CST) == 0) {
        /* all other users also made a copy in the meantime, so our new copy is unnecessary now */
        delete result;
        __atomic_store_n(&copyOnWriteCount, 1, __ATOMIC_SEQ_CST);
        return this;
    }

    __atomic_sub_fetch(&referenceCount, 1, __ATOMIC_SEQ_CST);
    return result;
}

void Frame::copyOnWriteDuplicate() noexcept {
    assert(!isForPhysicalAccess);
    __atomic_add_fetch(&copyOnWriteCount, 1, __ATOMIC_SEQ_CST);
    __atomic_add_fetch(&referenceCount, 1, __ATOMIC_SEQ_CST);
}

bool Frame::isCopyOnWrite() const noexcept {
    return __atomic_load_n(&copyOnWriteCount, __ATOMIC_SEQ_CST) != 1;
}

void Frame::pageIn(ProcessAddressSpace &addressSpace,
                   UserVirtualAddress virtualAddress,
                   Permissions mappingPermissions) {
    assert(addressSpace.lock.isLocked());
    assert(!isCopyOnWrite()
           || mappingPermissions == Permissions::READ ||
           mappingPermissions == Permissions::READ_EXECUTE);

    CacheType cacheType = isForPhysicalAccess ? CacheType::NONE : CacheType::NORMAL_WRITE_BACK;
    addressSpace.pagingContext.map(virtualAddress, address, mappingPermissions, cacheType);
}

PageOutContext Frame::pageOut(ProcessAddressSpace &addressSpace,
                              UserVirtualAddress address) noexcept {

    assert(addressSpace.lock.isLocked());

    addressSpace.pagingContext.unmap(address);

    scoped_lock sl1(KernelInterruptsLock);
    scoped_lock sl2(addressSpace.tlbIDsLock);

    PageOutContext pageOutContext(&addressSpace);
    for (TLBContextID tlbID: addressSpace.inTLBContextOfProcessor) {
        if (tlbID != TLB_CONTEXT_ID_NONE) {
            TLBContext &tlbContext = TLBContexts[tlbID];
            if (tlbContext.potentiallyHolds(&addressSpace.pagingContext)) {
                /* Optimization: if the addressSpace is not currently active on the Processor, we
                 * don't need to add the Frame to PageOutContext. It is garateed to be processed in
                 * returnToUserMode before the user space could abuse it to access the paged out
                 * frame. */
                bool isCurrentlyActive = false;
                {
                    Processor &processor = Processors[tlbContext.processorID];
                    scoped_lock sl(processor.scheduler.lock);
                    if (processor.activeThread() != nullptr && processor.activeThread()->process) {
                        isCurrentlyActive = processor.activeThread()->process->addressSpace == addressSpace;
                    }
                }

                assert(isCurrentlyActive || tlbContext.processorID != CurrentProcessor()->id);

                __atomic_add_fetch(&referenceCount, 1, __ATOMIC_SEQ_CST);
                PageOutContext pageOutAddition = tlbContext.requestInvalidate(
                            this, &addressSpace, address);

                if (isCurrentlyActive) {
                    pageOutContext |= pageOutAddition;
                }
           }
        }
    }
    return pageOutContext;
}

PhysicalAddress Frame::physicalAddress() const noexcept {
    return address;
}

void Frame::decreaseMemoryAreaReference() noexcept {
    __atomic_sub_fetch(&copyOnWriteCount, 1, __ATOMIC_SEQ_CST);
    size_t newRefCount = __atomic_sub_fetch(&referenceCount, 1, __ATOMIC_SEQ_CST);
    if (newRefCount == 0) {
        delete this;
    }
}

void Frame::decreaseInvalidateRequestReference() noexcept {
    size_t newRefCount = __atomic_sub_fetch(&referenceCount, 1, __ATOMIC_SEQ_CST);
    if (newRefCount == 0) {
        delete this;
    }
}

/* data access */
void Frame::copyFrom(uint8_t *destination, size_t offset, size_t length) noexcept {
    assert(offset + length <= PAGE_SIZE);

    uint8_t *directMapping = (address + offset).identityMapped().asPointer<uint8_t>();
    memcpy(destination, directMapping, length);
}

void Frame::copyTo(size_t offset, const uint8_t *source, size_t length) noexcept {
    assert(offset + length <= PAGE_SIZE);

    uint8_t *directMapping = (address + offset).identityMapped().asPointer<uint8_t>();
    memcpy(directMapping, source, length);
}

void Frame::copyFromMemoryArea(size_t frameOffset,
                               MemoryArea &memoryArea,
                               size_t areaOffset,
                               size_t length) {
    /* Danger Zone: be careful when introducing any locks here! The copyFrom call can actually
     * access this Frame again. */
    assert(frameOffset + length <= PAGE_SIZE);

    uint8_t *directMapping = (address + frameOffset).identityMapped().asPointer<uint8_t>();
    memoryArea._copyFrom(directMapping, areaOffset, length, {});
}

uint32_t Frame::atomicCopyFrom32(size_t offset) noexcept {
    uint32_t *directMapping = (address + offset).identityMapped().asPointer<uint32_t>();

    return __atomic_load_n(directMapping, __ATOMIC_SEQ_CST);
}

bool Frame::atomicCompareExchange32(size_t offset,
                                    uint32_t expectedValue,
                                    uint32_t newValue) noexcept {
    uint32_t *directMapping = (address + offset).identityMapped().asPointer<uint32_t>();

    return __atomic_compare_exchange_n(directMapping,
                                       &expectedValue,
                                       newValue,
                                       false,
                                       __ATOMIC_SEQ_CST,
                                       __ATOMIC_SEQ_CST);
}
