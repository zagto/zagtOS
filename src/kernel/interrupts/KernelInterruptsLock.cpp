#include <interrupts/KernelInterruptsLock.hpp>
#include <system/Processor.hpp>

KernelInterruptsLockClass KernelInterruptsLock;

static size_t initialSetupCounter = 1;

void KernelInterruptsLockClass::lock() {
    if (ProcessorsInitialized) [[likely]] {
        /* initial setup should leave at exactly the same amount of lock/unlock */
        assert(initialSetupCounter == 1);

        basicDisableInterrupts();
        CurrentProcessor()->interruptsLockValue++;
    } else {
        initialSetupCounter++;
    }
}

void KernelInterruptsLockClass::unlock() {
    if (ProcessorsInitialized) [[likely]] {
        Processor *processor = CurrentProcessor();
        assert(processor->interruptsLockValue > 0);
        processor->interruptsLockValue--;
        if (processor->interruptsLockValue == 0) {
            basicEnableInterrupts();
        }
    } else {
        /* a full unlock should not happen during initial setup */
        assert(initialSetupCounter > 1);
        initialSetupCounter--;
    }
}

bool KernelInterruptsLockClass::isLocked() const {
    if (ProcessorsInitialized) [[likely]] {
        return CurrentProcessor()->interruptsLockValue > 0;
    } else {
        return true;
    }
}

