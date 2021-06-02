#include <memory/PageOutContext.hpp>
#include <system/Processor.hpp>

PageOutContext::PageOutContext():
    timestamp{0} {}

PageOutContext::PageOutContext(CPUMask mask, uint64_t timestamp):
    mask{mask},
    timestamp{timestamp} {}

PageOutContext &PageOutContext::operator|=(const PageOutContext &other) {
    mask |= other.mask;
    timestamp = max(timestamp, other.timestamp);
    return *this;
}

void PageOutContext::realize() {
    for (Processor &processor: mask) {
        /* TODO: this could be done asynchronously if there are performance issues */
        processor.invalidateQueue.ensureProcessedUntil(timestamp);
    }
}
