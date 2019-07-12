#include <stdio.h>
#include <stdbool.h>
#include <acpi.h>

int main() {
    printf("Hello from ACPI\n");
    if (AcpiInitializeSubsystem()) {
        printf("AcpiInitializeSubsystem failed\n");
        return 1;
    }
    printf("ACPI Subsystem initialized\n");

    if (AcpiInitializeTables(NULL, 10, true)) {
        printf("AcpiInitializeTables failed\n");
        return 1;
    }
    printf("ACPI Tables initialized\n");

    ACPI_TABLE_MCFG *mcfg;
    if (AcpiGetTable("MCFG", 1, (ACPI_TABLE_HEADER **)&mcfg)) {
        printf("Getting MCFG table failed\n");
        return 1;
    }
}
