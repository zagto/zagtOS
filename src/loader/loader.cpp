#include <exit.hpp>
#include <log/Logger.hpp>
#include <Framebuffer.hpp>
#include <Files.hpp>
#include <MemoryMap.hpp>
#include <memory/VirtualMemory.hpp>
#include <memory/PhysicalMemory.hpp>
#include <ProgramBinary.hpp>


void LoaderMain() {
    cout << "Initializing..." << endl;
    hos_v1::FramebufferInfo &framebufferInfo = InitFramebuffer();

    cout << "Detecting Images..." << endl;
    const ProgramBinary kernel = LoadKernelImage();
    const ProgramBinary process = LoadProcessImage();
    cout << "Getting Memory Map..." << endl;

    memoryMap::freezeAndExitFirmware();

    cout << "Initialzing Memory Management..." << endl;
    PhysicalAddress maxPhysicalAddress = InitPhysicalFrameManagement();
    InitPaging();
    MapLoaderMemory();
    MapFramebufferMemory(framebufferInfo);
    CreateIdentityMap(maxPhysicalAddress);

    cout << "Setting up address space for kernel..." << endl;
    kernel.load(PagingContext::GLOBAL);
    cout << "Setting up address space for initial process..." << endl;
    process.load(PagingContext::PROCESS);
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
