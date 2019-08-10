#include <stdio.h>
#include <stdbool.h>
#include <acpi.h>
#include <zagtos/syscall.h>

#include <unistd.h>

void findProcessors(void) {
    ACPI_TABLE_MADT *madt;
    ACPI_STATUS result = AcpiGetTable(ACPI_SIG_MADT, 0, (ACPI_TABLE_HEADER **)&madt);
    if (result) {
        printf("Getting MADT table failed: %u\n", result);
        exit(1);
    }

    printf("Detecting secondary Processors...\n");

    char *pointer = (char *)madt + 0x2c;

    bool foundBootProcessor = false;

    while (pointer < (char *)madt + madt->Header.Length) {
        ACPI_SUBTABLE_HEADER *subTable = (ACPI_SUBTABLE_HEADER *)pointer;
        switch (subTable->Type) {
        case ACPI_MADT_TYPE_LOCAL_APIC: {
            ACPI_MADT_LOCAL_APIC *lapic = (ACPI_MADT_LOCAL_APIC *)subTable;
            if (lapic->LapicFlags & ACPI_MADT_ENABLED) {
                /* The first of these entries is the processor this code is running on, don't try
                 * start it */
                if (foundBootProcessor) {
                    printf("Found secondary processor - Processor ID: %u, APIC ID: %u\n",
                           lapic->ProcessorId,
                           lapic->Id);
                    zagtos_syscall(SYS_ADD_PROCESSOR, lapic->Id);
                } else {
                    foundBootProcessor = true;
                }
            }
        }
        }
        pointer += subTable->Length;
    }
}

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
    ACPI_STATUS result = AcpiGetTable(ACPI_SIG_MCFG, 0, (ACPI_TABLE_HEADER **)&mcfg);
    if (result == AE_NOT_FOUND) {
        printf("MCFG table does not exist\n");
        mcfg = NULL;
    } else if (result) {
        printf("Getting MCFG table failed: %u\n", result);
        return 1;
    }

    findProcessors();

    /*
     * TODO
    result = AcpiLoadTables();
    if (result) {
        printf("AcpiLoadTables failed: %u\n", result);
        return 1;
    }
    printf("ACPI Tables initialized\n");

    result = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (result) {
        printf("AcpiEnableSubsystem failed: %u\n", result);
        return 1;
    }
    printf("ACPI Subsystem initialized\n");

    result = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (result) {
        printf("AcpiInitializeObjects failed: %u\n", result);
        return 1;
    }
    printf("ACPI Objects initialized\n");
    */

}

