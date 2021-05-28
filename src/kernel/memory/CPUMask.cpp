#include <memory/CPUMask.hpp>
#include <system/System.hpp>

CPUMask::Iterator::Iterator(CPUMask &container, size_t position):
    container{container},
    position{position} {}

bool CPUMask::Iterator::isInRange() const {
    return position != CurrentSystem.numProcessors;
}

bool CPUMask::Iterator::operator!=(const Iterator &other) const {
    return position != other.position;
}

void CPUMask::Iterator::operator++() {
    assert(isInRange());
    position++;
    while (isInRange() && !container[position]) {
        position++;
    }
}

Processor &CPUMask::Iterator::operator*() {
    assert(isInRange());
    return Processors[position];
}

CPUMask::CPUMask() {}

CPUMask::CPUMask(const Processor &processor) {
    data[indexFor(processor.id)] = bitFor(processor.id);
}

size_t CPUMask::indexFor(size_t processorID) const {
    return processorID / PLATFORM_BITS;
}

size_t CPUMask::bitFor(size_t processorID) const {
    return 1u << (processorID% PLATFORM_BITS);
}

void CPUMask::operator|=(const CPUMask &other) {
    for (size_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        data[blockIndex] = other.data[blockIndex];
    }
}

bool CPUMask::operator[](size_t processorID) {
    return data[indexFor(processorID)] & bitFor(processorID);
}

CPUMask::Iterator CPUMask::begin() {
    return {*this, 0};
}

CPUMask::Iterator CPUMask::end() {
    return {*this, CurrentSystem.numProcessors};
}
