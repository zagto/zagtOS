#include <exit.hpp>
#include <log/Logger.hpp>
#include <Framebuffer.hpp>
#include <Files.hpp>
#include <MemoryMap.hpp>
#include <memory/VirtualMemory.hpp>
#include <memory/PhysicalMemory.hpp>


void LoaderMain() {
    cout << "Initializing..." << endl;
    hos_v1::FramebufferInfo &framebufferInfo = InitFramebuffer();

    cout << "Detecting Images..." << endl;
    /*void *kernel =*/ LoadKernelImage();
    /*void *sysEnvironment =*/ LoadProcessImage();
    cout << "Getting Memory Map..." << endl;

    memoryMap::freezeAndExitFirmware();

    cout << "Initialzing Memory Management..." << endl;
    PhysicalAddress maxPhysicalAddress = InitPhysicalFrameManagement();
    InitPaging();
    MapLoaderMemory();
    MapFramebufferMemory(framebufferInfo);
    CreateIdentityMap(maxPhysicalAddress);
    cout << "here\n";
    while(1);
    /*
    kernelEntry = LoadElfKernel(kernel);

    Log("Kernel Entry is ");
    LogUINTN(kernelEntry);
    Log("\n");

    EFI_PHYSICAL_ADDRESS acpiRoot = getACPIRoot(systemTable);

    Log("ACPI root is ");
    LogUINTN(acpiRoot);
    Log("\n");

    bootInfo = PrepareBootInfo(&initDataInfo, framebufferInfo, acpiRoot);

    Log("Exiting to Kernel...\n");
    ExitToKernel(kernelEntry, MasterPageTable, bootInfo);*/
}

void* __dso_handle = nullptr;
