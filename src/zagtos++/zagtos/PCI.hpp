#pragma once

#include <cstdint>
#include <zagtos/Messaging.hpp>

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
        return zbon::sizeFor(std::make_tuple(configBase, segmentNumber, busStart, busEnd));
    }
    void ZBONEncode(zbon::Encoder &encoder) {
        encoder.encodeValue(std::make_tuple(configBase, segmentNumber, busStart, busEnd));
    }
    bool ZBONDecode(zbon::Decoder &decoder) {
        return decoder.decodeFromTuple(configBase, segmentNumber, busStart, busEnd);
    }
};

}
}
