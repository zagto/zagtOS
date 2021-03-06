#include <memory/PageOutContext.hpp>
#include <system/Processor.hpp>
#include <processes/Process.hpp>

PageOutContext::PageOutContext(ProcessAddressSpace *addressSpace):
    addressSpace{addressSpace},
    timestamp{0} {}

PageOutContext::PageOutContext(ProcessAddressSpace *addressSpace, CPUMask mask, uint64_t timestamp):
    addressSpace{addressSpace},
    mask{mask},
    timestamp{timestamp} {}

PageOutContext &PageOutContext::operator|=(const PageOutContext &other) {
    assert(addressSpace == other.addressSpace);
    mask |= other.mask;
    timestamp = max(timestamp, other.timestamp);
    return *this;
}

void PageOutContext::realize() {
    for (Processor &processor: mask) {
        /* TODO: this could be done asynchronously if there are performance issues */
        scoped_lock sl(KernelInterruptsLock);

        scoped_lock sl2(processor.scheduler.lock);
        if (processor.activeThread()->process
                && &processor.activeThread()->process->addressSpace == addressSpace) {
            processor.invalidateQueue.ensureProcessedUntil(timestamp);
        }
    }
}
