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

struct BaseRegister {
    SharedMemory sharedMemory;
    size_t length;

    ZBON_ENCODING_FUNCTIONS(sharedMemory, length)
};

struct Device {
    uint64_t deviceID;
    std::array<std::optional<BaseRegister>, 5> BAR;

    ZBON_ENCODING_FUNCTIONS(deviceID, BAR)
};

}
}
