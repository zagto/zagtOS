#include <common/common.hpp>
#include <setup/BootInfo.hpp>
#include <system/System.hpp>
#include <lib/Vector.hpp>
#include <tasks/Task.hpp>
#include <tasks/ELF.hpp>
#include <tasks/Object.hpp>


extern "C" void _init();
extern "C" __attribute__((noreturn)) void switchStack(KernelStack *newStack,
                                                      void nextCode(BootInfo *),
                                                      BootInfo *nextCodeArg);

void KernelEntry2(BootInfo *bootInfo);


extern "C" __attribute__((noreturn)) void KernelEntry(BootInfo *bootInfo) {
    // Call global constructors
    _init();

    Log.init(bootInfo);
    Log << "Hello World. Log initialized." << EndLine;

    new (&CurrentSystem) System(bootInfo);
    Log << "System Object created." << EndLine;
    CurrentSystem.addBootProcessor();
    Log << "Processor added." << EndLine;

    switchStack(CurrentProcessor->kernelStack, KernelEntry2, bootInfo);
}

__attribute__((noreturn)) void KernelEntry2(BootInfo *bootInfoOld) {
    /* bootinfo may be in strange memory regions, so better copy it before switching to new
     * master page tables */
    BootInfo bootInfo = *bootInfoOld;

    Log << "Creating initial task..." << EndLine;

    Vector<u8> initFile(bootInfo.initDataInfo.size);
    Slice<Vector, u8> initSlice(&initFile);
    memcpy(&initFile[0],
           bootInfo.initDataInfo.address.identityMapped().asPointer<u8>(),
           bootInfo.initDataInfo.size);
    ELF initELF(initSlice);
    Object obj(INIT_MSG);
    new Task(initELF, Thread::Priority::FOREGROUND, &obj);

    /* the ELF data is the last thing we wanted to read from loader memory */
    CurrentSystem.kernelOnlyMasterPageTable->completelyUnmapLoaderRegion();

    Log << "Entering first task..." << EndLine;
    CurrentProcessor->interrupts.returnToUserMode();
}
