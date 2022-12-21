#include <zagtos/Firmware.hpp>
#include <zagtos/syscall.h>

namespace zagtos {

FirmwareInfo GetFirmwareInfo() {
    FirmwareInfo info;
    zagtos_syscall1(SYS_GET_FIRMWARE_INFO, reinterpret_cast<size_t>(&info));
    return info;
}

}
