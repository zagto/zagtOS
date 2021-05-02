#pragma once

#include <common/common.hpp>
#include <memory/CPUMask.hpp>

class PageOutContext {
private:
    CPUMask mask;
    uint64_t timestamp;

public:
    PageOutContext(CPUMask mask);
};
