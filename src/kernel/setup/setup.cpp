#include <common/common.hpp>
#include <setup/HandOverState.hpp>
#include <system/System.hpp>
#include <vector>
#include <processes/Process.hpp>
#include <system/Processor.hpp>


extern "C" void _init();

void KernelEntry2(void *handOver);


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

    CurrentProcessor->kernelStack->switchToKernelEntry(KernelEntry2, handOver);
}

__attribute__((noreturn)) void KernelEntry2(void *_handOver) {
    hos_v1::System *handOver = static_cast<hos_v1::System *>(_handOver);
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

    CurrentProcessor->scheduler.lock.lock();
    CurrentProcessor->scheduler.scheduleNext();
}
