#ifndef ACPI_H
#define ACPI_H

#include <efi.h>

EFI_PHYSICAL_ADDRESS getACPIRoot(EFI_SYSTEM_TABLE *systemTable);

#endif // ACPI_H
