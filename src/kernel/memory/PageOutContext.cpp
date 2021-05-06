#include <memory/PageOutContext.hpp>

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
