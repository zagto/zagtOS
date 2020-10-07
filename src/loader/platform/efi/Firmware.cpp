#include <Firmware.hpp>
#include <log/Logger.hpp>

using namespace efi;

static BOOLEAN GUIDEquals(EFI_GUID a, EFI_GUID b) {
    UINTN i;
    for (i = 0; i < sizeof(EFI_GUID); i++) {
        if (((char *)&a)[i] != ((char *)&b)[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

PhysicalAddress GetFirmwareRoot() {
    BOOLEAN found = FALSE;
    PhysicalAddress result;
    UINTN i;
    for (i = 0; i < SystemTable->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *table = &SystemTable->ConfigurationTable[i];

        EFI_GUID oldGUID = ACPI_TABLE_GUID;
        EFI_GUID newGIUD = ACPI_20_TABLE_GUID;

        if (!found && GUIDEquals(table->VendorGuid, oldGUID)) {
            result = reinterpret_cast<size_t>(table->VendorTable);
            found = TRUE;
        } else if (GUIDEquals(table->VendorGuid, newGIUD)) {
            result = reinterpret_cast<size_t>(table->VendorTable);
            found = TRUE;
            break;
        }
    }
    if (!found) {
        cout << "Unable to find ACPI root in EFI tables" << endl;
        Halt();
    }
    return result;
}

hos_v1::FirmwareType GetFirmwareType() {
    return hos_v1::FirmwareType::ACPI;
}
