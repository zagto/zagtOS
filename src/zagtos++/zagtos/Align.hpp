#pragma once

#include <cstddef>

namespace zagtos {

enum class AlignDirection {
    DOWN,
    UP
};

size_t align(size_t address, size_t alignment, AlignDirection direction);
void alignedShrink(size_t &start, size_t &length, size_t alignment);
void alignedGrow(size_t &start, size_t &length, size_t alignment);
}
