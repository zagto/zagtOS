#include <memory/CPUMask.hpp>
#include <system/System.hpp>

CPUMask::CPUMask() {}

CPUMask::CPUMask(Processor *processor) {
    data[processor->id / PLATFORM_BITS] = (1u << (processor->id % PLATFORM_BITS));
}

void CPUMask::operator|=(const CPUMask &other) {
    for (size_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        data[blockIndex] = other.data[blockIndex];
    }
}
