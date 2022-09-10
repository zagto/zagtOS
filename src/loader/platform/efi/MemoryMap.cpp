#include <common/inttypes.hpp>
#include <iostream>
#include <EFI.hpp>
#include <log/BasicLog.hpp>
#include <MemoryMap.hpp>

using namespace efi;

namespace memoryMap {

static EFI_MEMORY_DESCRIPTOR *map;
static size_t descriptorSize;
static size_t descriptorVersion;
static size_t numDescriptors;
static size_t currentIndex;


uint8_t *allocateHandOver(size_t numPages) {
    EFI_STATUS status;
    uint8_t *result;

    status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->AllocatePages),
                               4,
                               AllocateAnyPages,
                               EfiLoaderData,
                               numPages,
                               (EFI_PHYSICAL_ADDRESS *)&result);
    if (EFI_ERROR(status)) {
        cout << "Could not allocate memory for handover info: " << statusToString(status) << endl;
        Halt();
    }
    return result;
}

void freezeAndExitFirmware() {
    EFI_STATUS status;
    UINTN mapSize;
    UINTN numPages;
    EFI_MEMORY_DESCRIPTOR *memoryMap;
    UINTN mapKey;

    mapSize = 0;

    status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->GetMemoryMap),
                               5,
                               &mapSize,
                               nullptr,
                               &mapKey,
                               &descriptorSize,
                               &descriptorVersion);
    if (status != EFI_BUFFER_TOO_SMALL) {
        cout << "Could not get memory map size:" << statusToString(status) << endl;
        Halt();
    }

    // allocate more memory because memory map may become bigger when allocating memory
    mapSize += 0x2000;
    numPages = (mapSize + PAGE_SIZE - 1) / PAGE_SIZE;

    status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->AllocatePages),
                               4,
                               AllocateAnyPages,
                               EfiLoaderData,
                               numPages,
                               (EFI_PHYSICAL_ADDRESS *)&memoryMap);
    if (EFI_ERROR(status)) {
        cout << "Could not allocate memory for memory map: " << statusToString(status) << endl;
        Halt();
    }

    while (TRUE) {
        UINTN actualMapSize = mapSize;
        status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->GetMemoryMap),
                                   5,
                                   &actualMapSize,
                                   memoryMap,
                                   &mapKey,
                                   &descriptorSize,
                                   &descriptorVersion);
        if (EFI_ERROR(status)) {
            cout << "Could not get memory map: " << statusToString(status) << endl;
            Halt();
        }

        status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->ExitBootServices),
                                   2,
                                   ImageHandle,
                                   mapKey);
        if (status == EFI_INVALID_PARAMETER) {
            // prepare to do the same thing again
            /*status = uefi_call_wrapper(reinterpret_cast<void *>(ST->BootServices->FreePages),
                                       2,
                                       (EFI_PHYSICAL_ADDRESS)memoryMap,
                                       numPages);*/
        } else if (EFI_ERROR(status)) {
            cout << "Could exit boot services: " << statusToString(status) << endl;
            Halt();
        } else {
            map = memoryMap;

            basicLog::exitBootServices();

            /* mapSize should now hold the size of the memory map, so we can find out
             * how many descriptors it holds */
            if (actualMapSize % descriptorSize) {
                cout << "Memory Map Size not a Multiple of the Descriptor Size." << endl;
                Halt();
            }
            numDescriptors = actualMapSize / descriptorSize;
            return;
        }
    }
}


static EFI_MEMORY_DESCRIPTOR *getEntry(size_t index) {
    if (index >= numDescriptors) {
        cout << "Tried to get Memory Map Entry with Index Out of Range" << endl;
        Halt();
    }

    return (EFI_MEMORY_DESCRIPTOR *)((UINTN)map + descriptorSize * index);
}


static BOOLEAN EntryIsAvailable(EFI_MEMORY_DESCRIPTOR *entry) {
    // Not sure if this can happen, but be sure to ignore such entries
    if (entry->NumberOfPages == 0) {
        return FALSE;
    }

    switch (entry->Type) {
    case EfiConventionalMemory:
        return TRUE;
    default:
        return FALSE;
    }
}


static BOOLEAN EntryIsReclaimable(EFI_MEMORY_DESCRIPTOR *entry) {
    // Not sure if this can happen, but be sure to ignore such entries
    if (entry->NumberOfPages == 0) {
        return FALSE;
    }

    switch (entry->Type) {
    case EfiLoaderCode:
    case EfiLoaderData:
    /* boot services memory may be unmapped by the UEFI, so it is not available until our own page
     * tables are active */
    case EfiBootServicesCode:
    case EfiBootServicesData:
        return TRUE;
    default:
        return FALSE;
    }
}


optional<Region> firstAvailableRegion() {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = 0; index < numDescriptors; index++) {
        entry = getEntry(index);

        if (EntryIsAvailable(entry)) {
            currentIndex = index;
            return Region(entry->PhysicalStart, entry->NumberOfPages * PAGE_SIZE);
        }
    }
    return {};
}


optional<Region> nextAvailableRegion() {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = currentIndex + 1; index < numDescriptors; index++) {
        entry = getEntry(index);

        if (EntryIsAvailable(entry)) {
            currentIndex = index;
            return Region(entry->PhysicalStart, entry->NumberOfPages * PAGE_SIZE);
        }
    }
    return {};
}


optional<Region> firstReclaimableRegion() {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = 0; index < numDescriptors; index++) {
        entry = getEntry(index);

        if (EntryIsReclaimable(entry)) {
            currentIndex = index;
            return Region(entry->PhysicalStart, entry->NumberOfPages * PAGE_SIZE);
        }
    }
    return {};
}


optional<Region> nextReclaimableRegion() {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = currentIndex + 1; index < numDescriptors; index++) {
        entry = getEntry(index);

        if (EntryIsReclaimable(entry)) {
            currentIndex = index;
            return Region(entry->PhysicalStart, entry->NumberOfPages * PAGE_SIZE);
        }
    }
    return {};
}

}
