#include "zagtos/Align.hpp"

namespace zagtos {

size_t align(size_t address, size_t alignment, AlignDirection direction) {
    if (direction == AlignDirection::UP) {
        return address % alignment ? address + alignment - address % alignment : address;
    } else {
        return address - address % alignment;
    }
}

void alignedShrink(size_t &start, size_t &length, size_t alignment) {
    size_t newStart = align(start, alignment, AlignDirection::UP);
    if (length < newStart - start) {
        length = newStart; // TODO : 0????
    } else {
        length -= newStart - start;
        length = align(length, alignment, AlignDirection::DOWN);
    }
    start = newStart;
}


void alignedGrow(size_t &start, size_t &length, size_t alignment) {
    size_t newStart = align(start, alignment, AlignDirection::DOWN);
    length += start - newStart;
    length = align(length, alignment, AlignDirection::UP);
    start = newStart;
}

}
