#include <processes/KernelThreadEntry.hpp>
#include <processes/Scheduler.hpp>
#include <system/Processor.hpp>
#include <interrupts/KernelInterruptsLock.hpp>

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

    KernelInterruptsLock.unlock();

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
    /* The Thread was interrtupted, so interrupts should have been enabled */
    commonSetup();
    assert(registerState->interruptsFlagSet());
    CurrentProcessor->interruptsLockValue = 0;

    /* avoid overwriting the CurrentProcessor register with a copy from another processor */
    registerState->r15 = (size_t)CurrentProcessor;
}
