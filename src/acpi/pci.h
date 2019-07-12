#ifndef PCI_H
#define PCI_H

#include <acpi.h>
#include <stdint.h>

struct pci_segment_group {
    uint64_t config_base;
    uint16_t segement_number;
    uint8_t bus_start;
    uint8_t bus_end;
};

uint64_t pci_get_config_address(ACPI_PCI_ID *PciId);
extern struct pci_segment_group *pci_segment_groups;
extern uint64_t num_pci_segment_groups;


#endif // PCI_H
