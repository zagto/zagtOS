#pragma once

#include <common/common.hpp>
#include <system/System.hpp>

static_assert(MAX_NUM_PROCESSORS % PLATFORM_BITS == 0);

class CPUMask {
private:
    static constexpr size_t numBlocks = MAX_NUM_PROCESSORS / PLATFORM_BITS;
    uint32_t data[numBlocks]{0};

public:
    CPUMask();
    CPUMask(Processor *processor);
    void operator|=(const CPUMask &other);
};
