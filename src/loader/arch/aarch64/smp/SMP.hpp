#pragma once
#include <common/inttypes.hpp>

extern size_t BootProcessorHardwareID;

inline size_t secondaryProcessorEntryCodeLength() {
    return 1;
}
inline size_t startSecondaryProcessors() {
    return 1;
}
inline void releaseSecondaryProcessorsToKernel() {
}
