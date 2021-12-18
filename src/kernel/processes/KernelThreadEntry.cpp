#include <processes/KernelThreadEntry.hpp>
#include <processes/Scheduler.hpp>
#include <system/Processor.hpp>
#include <interrupts/KernelInterruptsLock.hpp>

extern "C" void basicIdleProcessor();

static void commonSetup() {
    Processor *processor = CurrentProcessor();
    if (processor->kernelStack) {
        processor->kernelStack->lock.unlock();
    }
    processor->kernelStack = processor->activeThread()->kernelStack;
    processor->scheduler.lock.unlock();
}

void IdleThreadEntry(void *) {
    commonSetup();
    KernelInterruptsLock.unlock();

    while (true) {
        basicIdleProcessor();
    }
}

void UserReturnEntry(void *) {
    commonSetup();
    CurrentProcessor()->returnToUserMode();
}

extern "C" void InKernelReturnEntryRestoreInterruptsLock(RegisterState *registerState) {
    /* The Thread was interrtupted, so interrupts should have been enabled */
    commonSetup();
    assert(registerState->interruptsFlagSet());
    CurrentProcessor()->interruptsLockValue = 0;
}
