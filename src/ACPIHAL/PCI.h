#ifndef PCI_H
#define PCI_H

#include <acpi.h>
#include <stdint.h>

uint64_t getPCIConfigAddress(ACPI_PCI_ID *PciId);
void initPCI(void);

#endif // PCI_H
