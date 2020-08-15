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

    static constexpr zbon::Type ZBONType() {
        return zbon::Type::OBJECT;
    }
    zbon::Size ZBONSize() const {
        return zbon::sizeForObject(configBase, segmentNumber, busStart, busEnd);
    }
    void ZBONEncode(zbon::Encoder &encoder) const {
        encoder.encodeObjectValue(configBase, segmentNumber, busStart, busEnd);
    }
    bool ZBONDecode(zbon::Decoder &decoder) {
        return decoder.decodeFromObject(configBase, segmentNumber, busStart, busEnd);
    }
};

struct BaseRegister {
    SharedMemory sharedMemory;
    size_t length;

    static constexpr zbon::Type ZBONType() {
        return zbon::Type::OBJECT;
    }
    zbon::Size ZBONSize() const {
        return zbon::sizeForObject(sharedMemory, length);
    }
    void ZBONEncode(zbon::Encoder &encoder) const {
        encoder.encodeObjectValue(sharedMemory, length);
    }
    bool ZBONDecode(zbon::Decoder &decoder) {
        return decoder.decodeFromObject(sharedMemory, length);
    }
};

struct Device {
    uint64_t deviceID;
    std::array<std::optional<BaseRegister>, 5> BAR;

    static constexpr zbon::Type ZBONType() {
        return zbon::Type::OBJECT;
    }
    zbon::Size ZBONSize() const {
        return zbon::sizeForObject(deviceID, BAR);
    }
    void ZBONEncode(zbon::Encoder &encoder) const {
        encoder.encodeObjectValue(deviceID, BAR);
    }
    bool ZBONDecode(zbon::Decoder &decoder) {
        return decoder.decodeFromObject(deviceID, BAR);
    }
};

}
}
