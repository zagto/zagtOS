#include <interrupts/KernelInterruptsLock.hpp>
#include <system/Processor.hpp>

KernelInterruptsLockClass KernelInterruptsLock;

static size_t initialSetupCounter = 1;

void KernelInterruptsLockClass::lock() {
    if (CurrentProcessor) [[likely]] {
        /* initial setup should leave at exactly the same amount of lock/unlock */
        assert(initialSetupCounter == 1);

        basicDisableInterrupts();
        CurrentProcessor->interruptsLockValue++;
    } else {
        initialSetupCounter++;
    }
}

void KernelInterruptsLockClass::unlock() {
    if (CurrentProcessor) [[likely]] {
        assert(CurrentProcessor->interruptsLockValue > 0);
        CurrentProcessor->interruptsLockValue--;
        if (CurrentProcessor->interruptsLockValue == 0) {
            basicEnableInterrupts();
        }
    } else {
        /* a full unlock should not happen during initial setup */
        assert(initialSetupCounter > 1);
        initialSetupCounter--;
    }
}

bool KernelInterruptsLockClass::isLocked() const {
    if (CurrentProcessor) [[likely]] {
        return CurrentProcessor->interruptsLockValue > 0;
    } else {
        return true;
    }
}

