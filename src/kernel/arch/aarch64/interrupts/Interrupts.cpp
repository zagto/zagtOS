#include <interrupts/Interrupts.hpp>
#include <interrupts/RegisterState.hpp>
#include <interrupts/util.hpp>
#include <iostream>
#include <processes/Process.hpp>
#include <processes/KernelThreadEntry.hpp>
#include <system/Processor.hpp>

enum ExceptionModeType {
    EL1T, EL1H, EL0_64, EL0_32
};

enum ExceptionTriggerType {
    SYNCRONOUS, IRQ, FIQ, ERROR
};

enum ExceptionClass {
    UNKNOWN                  = 0b000000,
    TRAPPED_SVE_SIMD_FP      = 0b000111,
    ILLEGAL_EXECUTION_STATE  = 0b001110,
    SVC_64                   = 0b010101,
    USER_INSTRUCTION_ABORT   = 0b100000,
    KERNEL_INSTRUCTION_ABORT = 0b100001,
    PC_ALIGNMENT_FAULT       = 0b100010,
    USER_DATA_ABORT          = 0b100100,
    KERNEL_DATA_ABORT        = 0b100101,
    SP_ALIGNMENT_FAULT       = 0b100110,
};

struct ExceptionInfo {
    RegisterState *registerState;
    uint64_t mode;
    uint64_t trigger;
    uint64_t exceptionClass;
    uint64_t specificSyndrome;
};

[[noreturn]]
void handleKernelSyncronousException(const ExceptionInfo &info) {
    cout << "Syncronous Exception in Kernel mode" << endl;
    if (info.exceptionClass == KERNEL_INSTRUCTION_ABORT) {
        cout << "attempt to fetch instruction from " << readFAR() << endl;
    }
    if (info.exceptionClass == KERNEL_DATA_ABORT) {
        const bool dataWriteNotRead = info.specificSyndrome & (1ul << 6);
        if (dataWriteNotRead) {
            cout << "attempt to write data to " << readFAR() << endl;
        } else {
            cout << "attempt to read data from " << readFAR() << endl;
        }
    }
    cout << *info.registerState << endl;
    Panic();
}

void handleUserSyncronousException(const ExceptionInfo &info) {
    assert(CurrentThread());
    assert(CurrentThread()->kernelStack->userRegisterState() == info.registerState);

    if (info.exceptionClass == USER_INSTRUCTION_ABORT || info.exceptionClass == USER_DATA_ABORT) {
        const uint64_t instructionFaultStatusCode = info.specificSyndrome & 0b111111;
        const bool dataWriteNotRead = info.specificSyndrome & (1ul << 6);

        Permissions permissions;
        if (info.exceptionClass == USER_INSTRUCTION_ABORT) {
            permissions = Permissions::READ_EXECUTE;
        } else {
            permissions = dataWriteNotRead ? Permissions::READ_WRITE : Permissions::READ;
        }

        if ((instructionFaultStatusCode >= 0b100 && instructionFaultStatusCode <= 0b111)
                || (instructionFaultStatusCode >= 0b1101 && instructionFaultStatusCode <= 0b1111)) {
            //cout << "Page fault with: " << *info.registerState << endl;
            /* translation fault */
            /* Page fault handling works with address spaces, which uses mutexes */
            KernelInterruptsLock.unlock();
            try {
                CurrentProcess()->addressSpace.handlePageFault(readFAR(), permissions);
            } catch (BadUserSpace &e) {
                /* try to core dump in case of BadUserSpace */
                try {
                    CurrentProcess()->addressSpace.coreDump(CurrentThread());
                }  catch (Exception &e) {
                    cout << "CoreDump failed: " << e.description() << endl;
                }
                KernelInterruptsLock.lock();
                throw;
            } catch (...) {
                KernelInterruptsLock.lock();
                throw;
            }
            KernelInterruptsLock.lock();

            cout << "handled user instruction fetch fault" << endl;
            return;
        }
    }
    cout << "Unhandled User Syncronous Exception: " << *info.registerState << endl;
    Panic();
}


[[noreturn]] void _handleInterrupt(RegisterState* registerState) {
    /* noreturn function. Do not use RAII in the main function scope. It will not be properly
     * destructed at the returnToUserMode/returnInsideKernelMode/scheduleNext call. */

    ExceptionInfo info = {
        .registerState = registerState,
        .mode = registerState->exceptionType >> 4,
        .trigger = registerState->exceptionType & 0xf,
        .exceptionClass = (registerState->exceptionSyndrome >> 26) & 0b111111,
        .specificSyndrome = registerState->exceptionSyndrome & 0xffffff,
    };

    Processor *processor = CurrentProcessor();

    /* fix CurrentProcessor->interruptsLockValue first! do not print anything before! */
    switch (info.mode) {
    case EL1T:
        processor->interruptsLockValue = 1;
        cout << "Exception coming from EL1T. This mode is not used by the kernel. This should not"
             << " happen." << endl;
        Panic();
        break;
    case EL1H:
        /* kernel space */
        /* Syncronous Exceptions can always occur, but are fatal in kernel mode. For non-syncronous
         * exceptions we care about the KernelInterruptsLock beeing unlocked as expected, so we
         * can return cleanly. */
        if (info.trigger == IRQ || info.trigger == FIQ) {
            assert(processor->interruptsLockValue == 0);
            assert(registerState->interruptsFlagSet());
        }

        /* Interrupts are disabled in Exception context, make our variable reflect that */
        processor->interruptsLockValue++;
        break;
    case EL0_64:
        /* user space */
        assert(processor->interruptsLockValue == 1);
        break;
    case EL0_32:
        processor->interruptsLockValue = 1;
        cout << "Exception coming from 32-bit EL0. This mode is not used by the kernel. This should"
             << " not happen." << endl;
        Panic();
        break;
    }
    bool fromUserSpace = info.mode == EL0_64;

    switch (info.trigger) {
    case SYNCRONOUS:
        cout << "Syncronous" << endl;
        break;
    case IRQ:
        cout << "IRQ" << endl;
        break;
    case FIQ:
        cout << "FIQ" << endl;
        break;
    case ERROR:
        cout << "Error" << endl;
        break;
    }

    cout << "Exception class: " << info.exceptionClass << " specific syndrome: "
         << info.specificSyndrome << endl;
    assert(registerState->fromSyscall == 0);
    Thread *thread = CurrentThread();
    assert(thread || (info.trigger != IRQ && info.trigger != FIQ && !fromUserSpace));

    Exception::Action action;
    do {
        action = Exception::Action::CONTINUE;
        try {
            if (info.trigger == SYNCRONOUS) {
                if (fromUserSpace) {
                    handleUserSyncronousException(info);
                } else {
                    handleKernelSyncronousException(info);
                }
            } else {
                cout << "Unknown Exception " << registerState << endl;
                Panic();
            }
        } catch (Exception &e) {
            action = e.handle();
        }
    } while (action == Exception::Action::RETRY);

    /* Page fault handling unlocks KernelInterruptsLock, so current Processor may have changed */
    processor = CurrentProcessor();

    assert(KernelInterruptsLock.isLocked());

    /* It is important to set these down here instead of at the beginning. Code like Page Fault
     * handling enables Interrupts, which could override these variables. */
    if (fromUserSpace) {
        thread->setKernelEntry(UserReturnEntry);
    } else {
        /* The only exception that should currently occur in in-kernel interrupts is
         * DiscardStateAndSchedule. In this case we can pass the currently interrupted register
         * state to InKernelReturnEntry to restore it.
         * Throwing the ThreadKilled/BadUserSpace exception here would be fatal, as these don't
         * leave the Thread in the Scheduler. The data on it's stack (including held locks,
         * references to heap objects) will be lost */
        thread->setKernelEntry(InKernelReturnEntry, registerState);
    }

    if (action == Exception::Action::CONTINUE) {
        assert(processor->interruptsLockValue == 1);
        if (fromUserSpace) {
            processor->returnToUserMode();
        } else {
            processor->interruptsLockValue--;
            /* We are returning from an interrupt, which means the interrupted code had interrupts
             * enabled. */
            processor->returnInsideKernelMode(registerState);
        }
    } else if (action == Exception::Action::SCHEDULE) {
        processor->scheduler.scheduleNext();
    }
    cout << "_handleInterrupt: should not reach here. " << endl;
    Panic();
}


/* called from exception vector */
extern "C" [[noreturn]] void handleInterrupt(RegisterState* registerState) {
    _handleInterrupt(registerState);
}
