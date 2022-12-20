#include <Serial.hpp>
#include <common/addresses.hpp>

static hos_v1::SerialInfo info;

hos_v1::SerialInfo &InitSerial() {
    info.type = hos_v1::SerialType::PC;
    info.baseAddress = PhysicalAddress::Null;
    info.memoryLength = 0;
    return info;
}
