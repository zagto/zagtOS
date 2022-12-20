#include <Serial.hpp>
#include <common/addresses.hpp>

static hos_v1::SerialInfo info;

hos_v1::SerialInfo &InitSerial() {
#ifdef __x86_64__
    info.type = hos_v1::SerialType::PC;
    info.baseAddress = PhysicalAddress::Null;
    info.memoryLength = 0;
    return info;
#else
#error "TODO: UART detection on non-x86 EFI platform"
#endif
}
