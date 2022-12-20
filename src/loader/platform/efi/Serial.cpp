#include <Serial.hpp>
#include <common/addresses.hpp>

static hos_v1::SerialInfo info;

hos_v1::SerialInfo &InitSerial() {
#ifdef SYSTEM_X86_64
    info.type = hos_v1::SerialType::PC;
    info.baseAddress = PhysicalAddress::Null;
    info.memoryLength = 0;
#else
#error "TODO: UART detection on non-x86 EFI platform"
#endif
}
