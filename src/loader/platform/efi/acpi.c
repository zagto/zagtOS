#include <util.h>
#include <acpi.h>

static BOOLEAN GUIDEquals(EFI_GUID a, EFI_GUID b) {
    UINTN i;
    for (i = 0; i < sizeof(EFI_GUID); i++) {
        if (((char *)&a)[i] != ((char *)&b)[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

EFI_PHYSICAL_ADDRESS getACPIRoot(EFI_SYSTEM_TABLE *systemTable) {
    BOOLEAN found = FALSE;
    EFI_PHYSICAL_ADDRESS result = 0;
    UINTN i;
    for (i = 0; i < systemTable->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *table = &systemTable->ConfigurationTable[i];

        EFI_GUID oldGUID = ACPI_TABLE_GUID;
        EFI_GUID newGIUD = ACPI_20_TABLE_GUID;

        if (!found && GUIDEquals(table->VendorGuid, oldGUID)) {
            result = (EFI_PHYSICAL_ADDRESS)table->VendorTable;
            found = TRUE;
        } else if (GUIDEquals(table->VendorGuid, newGIUD)) {
            result = (EFI_PHYSICAL_ADDRESS)table->VendorTable;
            found = TRUE;
            break;
        }
    }
    if (!found) {
        Log("Unable to find ACPI root in EFI tables\n");
        Halt();
    }
    return result;
}
