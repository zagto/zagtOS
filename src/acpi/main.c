#include <stdio.h>
#include <stdbool.h>
#include <acpi.h>

#include <unistd.h>

int main() {
    printf("Hello from ACPI\n");

    if (AcpiInitializeSubsystem()) {
        printf("AcpiInitializeSubsystem failed\n");
        return 1;
    }
    printf("ACPI Subsystem initialized\n");

    printf("AcpiOsGetRootPointer: %lu\n", AcpiOsGetRootPointer());

    if (AcpiInitializeTables(NULL, 10, true)) {
        printf("AcpiInitializeTables failed\n");
        return 1;
    }
    printf("ACPI Tables initialized\n");

    ACPI_TABLE_MCFG *mcfg;
    ACPI_STATUS result = AcpiGetTable(ACPI_SIG_MCFG, 0, (ACPI_TABLE_HEADER **)&mcfg);
    if (!ACPI_SUCCESS(result)) {
        printf("Getting MCFG table failed: %u\n", result);
        return 1;
    }
    printf("Got MCFG Table!\n");
}

