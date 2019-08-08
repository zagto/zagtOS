#include <common/common.hpp>
#include <setup/BootInfo.hpp>
#include <system/System.hpp>
#include <lib/vector.hpp>
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

    cout.init(bootInfo);
    cout << "Hello World. Log initialized." << endl;

    new (&CurrentSystem) System(bootInfo);
    cout << "System Object created." << endl;
    CurrentSystem.addBootProcessor();
    cout << "Processor added." << endl;

    switchStack(CurrentProcessor->kernelStack, KernelEntry2, bootInfo);
}

__attribute__((noreturn)) void KernelEntry2(BootInfo *bootInfoOld) {
    /* bootinfo may be in strange memory regions, so better copy it before switching to new
     * master page tables */
    BootInfo bootInfo = *bootInfoOld;

    cout << "Setting up time..." << endl;
    CurrentSystem.time.initialize();

    cout << "Creating initial task..." << endl;

    vector<uint8_t> initFile(bootInfo.initDataInfo.size);
    Slice<vector, uint8_t> initSlice(&initFile);
    memcpy(&initFile[0],
           bootInfo.initDataInfo.address.identityMapped().asPointer<uint8_t>(),
           bootInfo.initDataInfo.size);
    ELF initELF(initSlice);
    Object obj(INIT_MSG);
    new Task(initELF, Thread::Priority::FOREGROUND, &obj);

    /* the ELF data is the last thing we wanted to read from loader memory */
    CurrentSystem.kernelOnlyMasterPageTable->completelyUnmapLoaderRegion();

    cout << "Entering first task..." << endl;
    CurrentProcessor->interrupts.returnToUserMode();
}
