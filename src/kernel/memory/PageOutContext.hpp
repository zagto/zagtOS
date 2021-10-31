#pragma once

#include <common/common.hpp>
#include <memory/CPUMask.hpp>

class ProcessAddressSpace;

class PageOutContext {
private:
    const ProcessAddressSpace *addressSpace;
    CPUMask mask;
    uint64_t timestamp;

public:
    PageOutContext();
    PageOutContext(ProcessAddressSpace *addressSpace, CPUMask mask, uint64_t timestamp);
    PageOutContext &operator |=(const PageOutContext &other);

    void realize();
};
