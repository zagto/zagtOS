#pragma once

#include <common/common.hpp>
#include <memory/CPUMask.hpp>

class PageOutContext {
private:
    CPUMask mask;
    uint64_t timestamp;

public:
    PageOutContext();
    PageOutContext(CPUMask mask, uint64_t timestamp);
    PageOutContext &operator |=(const PageOutContext &other);

    void realize();
};
