#pragma once

#include <cstdint>
#include <zagtos/Messaging.hpp>

namespace zagtos {
namespace pci {

struct SegmentGroup {
    uint64_t configBase;
    uint16_t segementNumber;
    uint8_t busStart;
    uint8_t busEnd;
};

struct StartPCIManagerMessage {
    std::vector<SegmentGroup> segmentGroups;
    Port driverPort;

    size_t ZBONSize();
};

}
}
