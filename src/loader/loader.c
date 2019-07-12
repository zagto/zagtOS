#include <efi.h>
#include <efilib.h>
#include <util.h>
#include <file.h>
#include <efi-memory-map.h>
#include <physical-memory.h>
#include <paging.h>
#include <elf.h>
#include <framebuffer.h>
#include <bootinfo.h>
#include <exit.h>


EFI_STATUS EFIAPI efi_main (EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    EFI_FILE_PROTOCOL *volume;
    struct EfiMemoryMapInfo memoryMapInfo;
    struct ElfFileHeader *kernel;
    struct FramebufferInfo *framebufferInfo;
    struct InitDataInfo initDataInfo;
    const struct BootInfo *bootInfo;
    UINTN kernelEntry;
    EFI_PHYSICAL_ADDRESS maxPhysicalAddress;

    InitializeLib(imageHandle, systemTable);

    Log("Initializing...\n");
    framebufferInfo = InitFramebuffer();
    volume = FindOwnVolume(imageHandle);
    kernel = (struct ElfFileHeader *)LoadFile(volume, L"SHKERNEL.BIN", NULL);

    initDataInfo.address = (UINTN)LoadFile(volume, L"INITDATA.A", &initDataInfo.size);

    GetMemoryMapAndExitBootServices(imageHandle, &memoryMapInfo);
    LogExitBootServices();

    maxPhysicalAddress = InitPhysicalFrameManagement(&memoryMapInfo, &initDataInfo);
    Log("Maximum physical address: ");
    LogUINTN(maxPhysicalAddress);
    Log("\n");
    InitPaging();
    MapLoaderMemory(&memoryMapInfo);
    MapFramebufferMemory(framebufferInfo);
    CreateIdentityMap(maxPhysicalAddress);
    kernelEntry = LoadElfKernel(kernel);

    Log("Kernel Entry is ");
    LogUINTN(kernelEntry);
    Log("\n");

    bootInfo = PrepareBootInfo(&initDataInfo, framebufferInfo);

    Log("Exiting to Kernel...\n");
    ExitToKernel(kernelEntry, MasterPageTable, bootInfo);
}
