#pragma once

#include <cstdint>
#include <zagtos/SharedMemory.hpp>
#include <zagtos/UUID.hpp>
#include <array>
#include <optional>

namespace zagtos {
namespace pci {

static constexpr UUID MSG_ALLOCATE_MSI_IRQ(
            0x21, 0x67, 0x6e, 0x29, 0x48, 0xbc, 0x4a, 0x54,
            0x94, 0x00, 0x0b, 0x98, 0xcc, 0x6c, 0x64, 0x83);
static constexpr UUID MSG_ALLOCATE_MSI_IRQ_RESULT(
            0x21, 0x67, 0x6e, 0x29, 0x48, 0xbc, 0x4a, 0x54,
            0x94, 0x00, 0x0b, 0x98, 0xcc, 0x6c, 0x64, 0x83);
static constexpr UUID MSG_READ_CONFIG_SPACE(
            0xd6, 0xa6, 0x8b, 0x23, 0x5b, 0x16, 0x4d, 0x9d,
            0xb5, 0x6d, 0x49, 0x11, 0xd0, 0x4a, 0x47, 0x5a);
static constexpr UUID MSG_READ_CONFIG_SPACE_RESULT(
            0x16, 0xb7, 0xa6, 0x11, 0xaf, 0x21, 0x4d, 0xed,
            0xab, 0xef, 0x2d, 0x00, 0x55, 0x92, 0xed, 0x3d);
static constexpr UUID MSG_WRITE_CONFIG_SPACE(
            0xd5, 0xbc, 0x3b, 0x54, 0x78, 0xeb, 0x43, 0x38,
            0x80, 0x09, 0xa8, 0x64, 0x69, 0x37, 0xd7, 0x04);
static constexpr UUID MSG_WRITE_CONFIG_SPACE_RESULT(
            0x66, 0x26, 0x25, 0x03, 0x22, 0xf0, 0x45, 0x7b,
            0x91, 0x8c, 0x63, 0x97, 0xb1, 0xcd, 0xa1, 0xb7);

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
    uint64_t combinedID;
    std::array<std::optional<SharedMemory>, 6> BAR;
    std::vector<uint32_t> supportedCapabilities;

    uint16_t vendorID() const {
        return static_cast<uint16_t>(combinedID);
    }
    uint16_t deviceID() const {
        return static_cast<uint16_t>(combinedID >> 16);
    }
    uint8_t revisionID() const {
        return static_cast<uint16_t>(combinedID >> 32);
    }
    uint8_t progIF() const {
        return static_cast<uint16_t>(combinedID >> 40);
    }
    uint8_t subClass() const {
        return static_cast<uint16_t>(combinedID >> 48);
    }
    uint8_t classCode() const {
        return static_cast<uint16_t>(combinedID >> 56);
    }

    ZBON_ENCODING_FUNCTIONS(combinedID, BAR, supportedCapabilities)
};

}
}
