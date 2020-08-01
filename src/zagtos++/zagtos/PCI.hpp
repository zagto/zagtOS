#pragma once

#include <cstdint>
#include <zagtos/Messaging.hpp>
#include <array>

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

struct Device {
    uint64_t deviceID;
    std::array<SharedMemory, 5> BAR;
    std::array<size_t, 5> BARLength;

    static constexpr zbon::Type ZBONType() {
        return zbon::Type::OBJECT;
    }
    zbon::Size ZBONSize() const {
        return zbon::sizeForObject(deviceID, BAR, BARLength);
    }
    void ZBONEncode(zbon::Encoder &encoder) const {
        encoder.encodeObjectValue(deviceID, BAR, BARLength);
    }
    bool ZBONDecode(zbon::Decoder &decoder) {
        return decoder.decodeFromObject(deviceID, BAR, BARLength);
    }
};

}
}
