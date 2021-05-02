#include <processes/ProcessAddressSpace.hpp>
#include <processes/MappedArea.hpp>

Result<shared_ptr<MemoryArea> &> ProcessAddressSpace::findMemoryArea(UserVirtualAddress virtualAddress) {
    MappedArea *mappedArea = mappedAreas.findMappedArea(virtualAddress);
    if (mappedArea == nullptr) {
        return Status::BadUserSpace();
    }
    return mappedArea->memoryArea;
}


/* PhysicalAddress returned for Futex manager use only. May have changed already since
 * ProcessAddressSpace is unlocked! */
Result<uint32_t> ProcessAddressSpace::atomicCopyFrom32(size_t address) {
    scoped_lock sl(lock);

    Result<PhysicalAddress> physical = pagingContext.resolve(UserVirtualAddress(address));
    if (!physical) {
        return physical.status();
    }
    volatile uint32_t *directValue = physical->identityMapped().asPointer<volatile uint32_t>();
    return *directValue;
}

/* PhysicalAddress returned for Futex manager use only. May have changed already since
 * ProcessAddressSpace is unlocked! */
Result<uint32_t> ProcessAddressSpace::atomicCompareExchange32(size_t address,
                                                              uint32_t expectedValue,
                                                              uint32_t newValue) {
    scoped_lock sl(lock);

    Result<PhysicalAddress> physical = pagingContext.resolve(UserVirtualAddress(address));
    if (!physical) {
        return physical.status();
    }
    volatile uint32_t *directValue = physical->identityMapped().asPointer<volatile uint32_t>();
    return compare_exchange_u32(*directValue, expectedValue, newValue);
}

/* PhysicalAddress returned for Futex manager use only. May have changed already since
 * ProcessAddressSpace is unlocked! */
Result<PhysicalAddress> ProcessAddressSpace::resolve(UserVirtualAddress virtualAddress) {
    scoped_lock sl(lock);
    return _resolve(virtualAddress);
}
