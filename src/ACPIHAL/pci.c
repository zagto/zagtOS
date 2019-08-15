#include <stdio.h>
#include <stdlib.h>
#include <pci.h>

struct pci_segment_group *pci_segment_groups = NULL;
uint64_t num_pci_segment_groups = 0;


uint64_t pci_get_config_address(ACPI_PCI_ID *PciId) {
    if (!pci_segment_groups) {
        printf("pci_get_config_address called before pci was detected\n");
        exit(1);
    }

    for (size_t i = 0; i < num_pci_segment_groups; i++) {
        if (pci_segment_groups[i].segement_number == PciId->Segment) {
            struct pci_segment_group *group = &pci_segment_groups[i];

            if (PciId->Bus < group->bus_start || PciId->Bus > group->bus_end) {
                printf("ACPI: pci_get_config_address called for non existing segment group %u\n",
                       (unsigned int)PciId->Segment);
                exit(1);
            }

            return group->config_base + (((uint64_t)PciId->Bus - group->bus_start) << 20
                                         | (uint64_t)PciId->Device << 15
                                         | (uint64_t)PciId->Function << 12);
        }
    }
    printf("ACPI: pci_get_config_address called for non existing segment group %u\n",
           (unsigned int)PciId->Segment);
    exit(1);
}
