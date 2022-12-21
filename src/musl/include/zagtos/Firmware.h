#ifndef __ZAGTOS_ACPI_H
#define __ZAGTOS_ACPI_H

#define KERNEL_API_ONLY_FIRMWARE_INFO
#include <zagtos/KernelApi.h>
#undef KERNEL_API_ONLY_FIRMWARE_INFO

struct ZoFirmwareInfo ZoGetFirmwareInfo(void);

#endif
