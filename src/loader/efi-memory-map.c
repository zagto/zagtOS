#include <efi.h>
#include <efilib.h>
#include <util.h>
#include <efi-memory-map.h>


void GetMemoryMapAndExitBootServices(EFI_HANDLE imageHandle, struct EfiMemoryMapInfo *mapInfo) {
    EFI_STATUS status;
    UINTN mapSize;
    UINTN numPages;
    EFI_MEMORY_DESCRIPTOR *memoryMap;
    UINTN mapKey;
    UINTN descriptorSize;
    UINTN descriptorVersion;

    while (TRUE) {
        mapSize = 0;

        status = uefi_call_wrapper(ST->BootServices->GetMemoryMap,
                                   5,
                                   &mapSize,
                                   NULL,
                                   &mapKey,
                                   &descriptorSize,
                                   &descriptorVersion);
        if (status != EFI_BUFFER_TOO_SMALL) {
            LogStatus("Could not get memory map size", status);
            Halt();
        }

        // allocate more memory because memory map may become bigger when allocating memory
        mapSize += 0x200;
        numPages = (mapSize + PAGE_SIZE - 1) / PAGE_SIZE;

        status = uefi_call_wrapper(ST->BootServices->AllocatePages,
                                   4,
                                   AllocateAnyPages,
                                   EfiLoaderData,
                                   numPages,
                                   (EFI_PHYSICAL_ADDRESS *)&memoryMap);
        if (EFI_ERROR(status)) {
            LogStatus("Could not allocate memory for memory map: %lu", status);
            Halt();
        }

        status = uefi_call_wrapper(ST->BootServices->GetMemoryMap,
                                   5,
                                   &mapSize,
                                   memoryMap,
                                   &mapKey,
                                   &descriptorSize,
                                   &descriptorVersion);
        if (EFI_ERROR(status)) {
            LogStatus("Could not get memory map", status);
            Halt();
        }

        status = uefi_call_wrapper(ST->BootServices->ExitBootServices,
                                   2,
                                   imageHandle,
                                   mapKey);
        if (status == EFI_INVALID_PARAMETER) {
            // prepare to do the same thing again
            status = uefi_call_wrapper(ST->BootServices->FreePages,
                                       2,
                                       (EFI_PHYSICAL_ADDRESS)memoryMap,
                                       numPages);
        } else if (EFI_ERROR(status)) {
            LogStatus("Could exit boot services", status);
            Halt();
        } else {
            mapInfo->map = memoryMap;
            mapInfo->descriptorSize = descriptorSize;
            mapInfo->descriptorVersion = descriptorVersion;

            /* mapSize should now hold the size of the memory map, so we can find out
             * how many descriptors it holds */
            if (mapSize % mapInfo->descriptorSize) {
                Log("Memory Map Size not a Multiple of the Descriptor Size");
                Halt();
            }
            mapInfo->numDescriptors = mapSize / mapInfo->descriptorSize;
            return;
        }
    }
}


static EFI_MEMORY_DESCRIPTOR *GetEntry(struct EfiMemoryMapInfo *mapInfo, UINTN index) {
    if (index >= mapInfo->numDescriptors) {
        Log("Tried to get Memory Map Entry with Index Out of Range");
        Halt();
    }

    return (EFI_MEMORY_DESCRIPTOR *)((UINTN)mapInfo->map + mapInfo->descriptorSize * index);
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


EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetFirstAvailableEntry(struct EfiMemoryMapInfo *mapInfo) {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = 0; index < mapInfo->numDescriptors; index++) {
        entry = GetEntry(mapInfo, index);

        if (EntryIsAvailable(entry)) {
            mapInfo->currentIndex = index;
            return entry;
        }
    }
    return NULL;
}


EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetNextAvailableEntry(struct EfiMemoryMapInfo *mapInfo) {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = mapInfo->currentIndex + 1; index < mapInfo->numDescriptors; index++) {
        entry = GetEntry(mapInfo, index);

        if (EntryIsAvailable(entry)) {
            mapInfo->currentIndex = index;
            return entry;
        }
    }
    return NULL;
}


EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetFirstReclaimableEntry(struct EfiMemoryMapInfo *mapInfo) {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = 0; index < mapInfo->numDescriptors; index++) {
        entry = GetEntry(mapInfo, index);

        if (EntryIsReclaimable(entry)) {
            mapInfo->currentIndex = index;
            return entry;
        }
    }
    return NULL;
}


EFI_MEMORY_DESCRIPTOR *EfiMemoryMapGetNextReclaimableEntry(struct EfiMemoryMapInfo *mapInfo) {
    EFI_MEMORY_DESCRIPTOR *entry;
    UINTN index;

    for (index = mapInfo->currentIndex + 1; index < mapInfo->numDescriptors; index++) {
        entry = GetEntry(mapInfo, index);

        if (EntryIsReclaimable(entry)) {
            mapInfo->currentIndex = index;
            return entry;
        }
    }
    return NULL;
}

