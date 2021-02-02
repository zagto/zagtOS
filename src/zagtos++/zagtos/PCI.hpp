#pragma once

#include <cstdint>
#include <zagtos/Messaging.hpp>
#include <array>
#include <optional>

namespace zagtos {
namespace pci {

struct SegmentGroup {
    uint64_t configBase;
    uint16_t segmentNumber;
    uint8_t busStart;
    uint8_t busEnd;

    ZBON_ENCODING_FUNCTIONS(configBase, segmentNumber, busStart, busEnd)
};

enum Capability {
    POWER_MANAGEMENT = 0x01,
    MSI_X = 0x11
};

struct Device {
    uint64_t deviceID;
    std::array<std::optional<SharedMemory>, 6> BAR;

    ZBON_ENCODING_FUNCTIONS(deviceID, BAR)
};

}
}
