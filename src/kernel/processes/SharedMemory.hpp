#pragma once

#include <common/common.hpp>
#include <processes/MappedArea.hpp>

struct SharedMemory {
    MappedArea::Source source;
    PhysicalAddress physicalAddress;
    size_t length;

    SharedMemory(MappedArea::Source source, PhysicalAddress physicalAddress, size_t length):
        source{source},
        physicalAddress{physicalAddress},
        length{length} {}

    SharedMemory(const hos_v1::SharedMemory &handOver):
        source{static_cast<MappedArea::Source>(handOver.source)},
        physicalAddress{handOver.physicalStart},
        length{handOver.length} {}
};

