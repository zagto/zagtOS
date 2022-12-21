#include <zagtos/Firmware.h>
#include <syscall.h>

struct ZoFirmwareInfo ZoGetFirmwareInfo(void) {
    struct ZoFirmwareInfo info;
    zagtos_syscall1(SYS_GET_FIRMWARE_INFO, (size_t)&info);
    return info;
}
