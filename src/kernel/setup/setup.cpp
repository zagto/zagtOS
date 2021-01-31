#include <common/common.hpp>
#include <setup/HandOverState.hpp>
#include <system/System.hpp>
#include <vector>
#include <processes/Process.hpp>


extern "C" void _init();
extern "C" __attribute__((noreturn)) void switchStack(uint8_t *newStack,
                                                      void nextCode(hos_v1::System *),
                                                      hos_v1::System *nextCodeArg);

void KernelEntry2(hos_v1::System *handOver);
void KernelEntrySecondaryProcessor2(hos_v1::System *handOver);


extern "C" __attribute__((noreturn)) void KernelEntry(hos_v1::System *handOver) {
    CurrentProcessor = nullptr;

    /* Call global constructors */
    _init();

    cout.init(handOver->framebufferInfo);
    cout << "Hello World. Log initialized." << endl;

    new (&CurrentSystem) System(*handOver);
    cout << "System Object created." << endl;

    CurrentSystem.addBootProcessor();
    cout << "Processor added." << endl;

    /* TODO: wait for eventual already running secondary processors */

    switchStack(CurrentProcessor->kernelStack, KernelEntry2, handOver);
}

__attribute__((noreturn)) void KernelEntry2(hos_v1::System *handOver) {
    {
        cout << "Setting up time..." << endl;
        CurrentSystem.time.initialize();

        cout << "Creating initial processes..." << endl;
        handOver->decodeProcesses();

        /* the ELF data is the last thing we wanted to read from loader memory */
        //CurrentSystem.kernelOnlyPagingContext.completelyUnmapLoaderRegion();

        cout << "Entering first process..." << endl;
    }
    /* TODO: clear handover state from the lower half of kernelOnlyPagingContext */
    CurrentProcessor->interrupts.returnToUserMode();
}

extern "C" __attribute__((noreturn)) void KernelEntrySecondaryProcessor() {
    assert(CurrentSystem.processorsLock.isLocked());

    CurrentProcessor = new Processor(false);

    /* create local variable as CurrentProcessor is a register variable that can't be passed by
     * const reference for push_back */
    Processor *tmp = CurrentProcessor;
    CurrentSystem.processors.push_back(tmp);

    switchStack(CurrentProcessor->kernelStack, KernelEntrySecondaryProcessor2, nullptr);
}

__attribute__((noreturn)) void KernelEntrySecondaryProcessor2(hos_v1::System *) {
    /* we now use our own stack, free to add more processors */
    CurrentSystem.processorsLock.unlock();

    cout << "started processor " << (CurrentSystem.processors.size() - 1) << endl;
    CurrentProcessor->interrupts.returnToUserMode();
}
