#include <common/common.hpp>
#include <setup/HandOverState.hpp>
#include <system/System.hpp>
#include <vector>
#include <processes/Process.hpp>
#include <system/Processor.hpp>


extern "C" void _init();
extern "C" __attribute__((noreturn)) void switchStack(uint8_t *newStack,
                                                      void nextCode(hos_v1::System *),
                                                      hos_v1::System *arg);

void KernelEntry2(hos_v1::System *handOver);
void KernelEntrySecondaryProcessor2(hos_v1::System *handOver);


static size_t secondaryProcessorsStartLock = 1;
static size_t processorsStarted = 1;

extern "C" __attribute__((noreturn))
void KernelEntry(hos_v1::System *handOver, size_t processorID, size_t hardwareID) {
    if (processorID == 0) {
        CurrentProcessor = nullptr;

        /* glocal constructor for System and cout needs this */
        _HandOverSystem = handOver;

        /* Call global constructors */
        _init();

        cout << "Hello World. Log initialized." << endl;

        Status status = CurrentSystem.initProcessorsAndTLB();
        if (!status) {
            cout << "Exception during boot processors initialization" << endl;
            Panic();
        }
        cout << "Processors initialized." << endl;
    } else {
        while (__atomic_load_n(&secondaryProcessorsStartLock, __ATOMIC_SEQ_CST) != 0) {
            /* wait for first processor to clear start lock */
        }
    }

    CurrentProcessor = &Processors[processorID];
    CurrentProcessor->hardwareID = hardwareID;
    cout << "Processor " << processorID << " hardwareID "  << hardwareID << " running." << endl;
    assert(CurrentProcessor->id == processorID);

    switchStack(CurrentProcessor->kernelStack, KernelEntry2, handOver);
}

__attribute__((noreturn)) void KernelEntry2(hos_v1::System *handOver) {
    size_t processorID = CurrentProcessor->id;

    CurrentSystem.setupCurrentProcessor();
    CurrentProcessor->localInitialization();

    if (processorID == 0) {
        cout << "Setting up time..." << endl;
        CurrentSystem.time.initialize();

        cout << "Creating initial processes..." << endl;
        handOver->decodeProcesses();

        cout << "Unlocking secondary processors..." << endl;
        __atomic_store_n(&processorsStarted, 1, __ATOMIC_SEQ_CST);
        __atomic_store_n(&secondaryProcessorsStartLock, 0, __ATOMIC_SEQ_CST);

        size_t startedCount = 0;
        while (startedCount != CurrentSystem.numProcessors) {
            startedCount = __atomic_load_n(&processorsStarted, __ATOMIC_SEQ_CST);
        }

        cout << "All processors started: TODO: unlock loader memory" << endl;
        /* TODO: clear handover state from the lower half of kernelOnlyPagingContext */
        /* the ELF data is the last thing we wanted to read from loader memory */
        //CurrentSystem.kernelOnlyPagingContext.completelyUnmapLoaderRegion();
    } else {
        __atomic_add_fetch(&processorsStarted, 1,  __ATOMIC_SEQ_CST);
    }

    {
        cout << "Processor " << processorID << " entering normal operation" << endl;
    }
    CurrentProcessor->returnToUserMode();
}
