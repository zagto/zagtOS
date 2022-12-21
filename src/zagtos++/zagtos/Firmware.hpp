#pragma once

#define KERNEL_API_ONLY_FIRMWARE_INFO
#include <zagtos/KernelApi.h>
#undef KERNEL_API_ONLY_FIRMWARE_INFO

namespace zagtos {
using FirmwareInfo = cApi::ZoFirmwareInfo;

FirmwareInfo GetFirmwareInfo();
}
