#include <common/common.hpp>
#include <setup/BootInfo.hpp>
#include <system/System.hpp>
#include <lib/vector.hpp>
#include <tasks/Task.hpp>
#include <tasks/ELF.hpp>


extern "C" void _init();
extern "C" __attribute__((noreturn)) void switchStack(KernelStack *newStack,
                                                      void nextCode(BootInfo *),
                                                      BootInfo *nextCodeArg);

void KernelEntry2(BootInfo *bootInfo);
void KernelEntrySecondaryProcessor2(BootInfo *);


extern "C" __attribute__((noreturn)) void KernelEntry(BootInfo *bootInfo) {
    CurrentProcessor = nullptr;

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

    {
        cout << "Setting up time..." << endl;
        CurrentSystem.time.initialize();

        cout << "Creating initial task..." << endl;

        vector<uint8_t> initFile(bootInfo.initDataInfo.size);
        Slice<vector, uint8_t> initSlice(&initFile);
        memcpy(&initFile[0],
               bootInfo.initDataInfo.address.identityMapped().asPointer<uint8_t>(),
               bootInfo.initDataInfo.size);
        ELF initELF(initSlice);

        const uint8_t uuidData[] = {0x72, 0x75, 0xb0, 0x4d, 0xdf, 0xc1, 0x41, 0x18,
                                           0xba, 0xbd, 0x0b, 0xf3, 0xfb, 0x79, 0x8e, 0x55};
        const UUID beInitMessage(uuidData);

        Task *newTask = new Task(initELF, Thread::Priority::FOREGROUND, beInitMessage, 0);
        LockHolder lh(newTask->pagingLock);

        /* the ELF data is the last thing we wanted to read from loader memory */
        //CurrentSystem.kernelOnlyPagingContext.completelyUnmapLoaderRegion();

        cout << "Entering first task..." << endl;
    }
    CurrentProcessor->interrupts.returnToUserMode();
}

extern "C" __attribute__((noreturn)) void KernelEntrySecondaryProcessor() {
    assert(CurrentSystem.processorsLock.isLocked());

    CurrentProcessor = new Processor(false);
    CurrentSystem.processors.push_back(CurrentProcessor);

    switchStack(CurrentProcessor->kernelStack, KernelEntrySecondaryProcessor2, nullptr);
}

__attribute__((noreturn)) void KernelEntrySecondaryProcessor2(BootInfo *) {
    /* we now use our own stack, free to add more processors */
    CurrentSystem.processorsLock.unlock();

    cout << "started processor " << (CurrentSystem.processors.size() - 1) << endl;
    CurrentProcessor->interrupts.returnToUserMode();
}
