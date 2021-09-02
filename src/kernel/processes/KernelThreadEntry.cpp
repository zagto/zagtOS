#include <processes/KernelThreadEntry.hpp>
#include <processes/Scheduler.hpp>
#include <system/Processor.hpp>

extern "C" void basicIdleProcessor();

static void commonSetup() {
    Scheduler &scheduler = CurrentProcessor->scheduler;
    if (CurrentProcessor->kernelStack) {
        CurrentProcessor->kernelStack->lock.unlock();
    }
    CurrentProcessor->kernelStack = scheduler.activeThread()->kernelStack;
    scheduler.lock.unlock();
}

void IdleThreadEntry(void *) {
    commonSetup();

    cout << "Hello World from Idle Thread on Processor " << CurrentProcessor->id << endl;

    CurrentProcessor->kernelInterruptsLock.unlock();

    while (true) {
        basicIdleProcessor();
    }
}

void UserReturnEntry(void *) {
    commonSetup();

    cout << "Hello World from Regular Thread on Processor " << CurrentProcessor->id << endl;

    CurrentProcessor->returnToUserMode();
}

extern "C" void InKernelReturnEntryRestoreInterruptsLock(RegisterState *registerState) {
    CurrentProcessor->interruptsLockLocked = !registerState->interruptsFlagSet();
}
