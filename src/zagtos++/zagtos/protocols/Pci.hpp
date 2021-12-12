#pragma once

#include <cstdint>
#include <zagtos/Messaging.hpp>
#include <zagtos/UUID.hpp>
#include <array>
#include <optional>

namespace zagtos {
namespace pci {

static constexpr UUID MSG_START_PCI_DRIVER(
            0x7d, 0xc3, 0xc5, 0x9a, 0xde, 0x8b, 0x42, 0x05,
            0x87, 0x62, 0x73, 0x6e, 0x80, 0x8d, 0xfe, 0xed);
static constexpr UUID MSG_ALLOCATE_MSI_IRQ(
            0x21, 0x67, 0x6e, 0x29, 0x48, 0xbc, 0x4a, 0x54,
            0x94, 0x00, 0x0b, 0x98, 0xcc, 0x6c, 0x64, 0x83);

struct SegmentGroup {
    uint64_t configBase;
    uint16_t segmentNumber;
    uint8_t busStart;
    uint8_t busEnd;

    ZBON_ENCODING_FUNCTIONS(configBase, segmentNumber, busStart, busEnd)
};

enum Capability {
    POWER_MANAGEMENT = 0x01,
    MSI = 0x05,
    MSI_X = 0x11,
};

struct Device {
    uint64_t deviceID;
    std::array<std::optional<SharedMemory>, 6> BAR;
    std::vector<uint32_t> supportedCapabilities;

    ZBON_ENCODING_FUNCTIONS(deviceID, BAR, supportedCapabilities)
};

}
}
