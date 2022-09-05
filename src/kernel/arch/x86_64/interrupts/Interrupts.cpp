#include <common/common.hpp>
#include <system/System.hpp>
#include <interrupts/Interrupts.hpp>
#include <interrupts/ContextSwitch.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <system/Processor.hpp>
#include <processes/KernelThreadEntry.hpp>


[[noreturn]] void handleKernelException(RegisterState *registerState) noexcept {
    switch (registerState->intNr) {
    default:
        cout << "x86 Exception occured In Kernel Mode:" << *registerState
             << " on Processor " << CurrentProcessor()->id << endl;
        Panic();
    }
}

void handleUserException(RegisterState *registerState) {
    assert(CurrentThread());
    assert(CurrentThread()->kernelStack->userRegisterState() == registerState);

    if (registerState->intNr == StaticInterrupt::PAGE_FAULT) [[likely]] {
        static const size_t PAGING_ERROR_FLAGS{0b11111};
        static const size_t PAGING_ERROR_INACTIVE{0b1};
        static const size_t PAGING_ERROR_WRITE{0b10};
        static const size_t PAGING_ERROR_USER{0b100};
        static const size_t PAGING_ERROR_INSTRUCTION_FETCH{0b10000};

        size_t cr2 = readCR2();

        size_t errorFlags{registerState->errorCode & PAGING_ERROR_FLAGS};
        if (!(errorFlags & PAGING_ERROR_USER)) {
            cout << "userHandler called for non user page fault" << endl;
            Panic();
        }

        Permissions requiredPermissions;
        switch (errorFlags & ~PAGING_ERROR_INACTIVE) {
        case PAGING_ERROR_USER:
            requiredPermissions = Permissions::READ;
            break;
        case PAGING_ERROR_USER | PAGING_ERROR_WRITE:
            requiredPermissions = Permissions::READ_WRITE;
            break;
        case PAGING_ERROR_USER | PAGING_ERROR_INSTRUCTION_FETCH:
            requiredPermissions = Permissions::READ_EXECUTE;
            break;
        default:
            cout << "Unexpected Page Fault Flags: " << errorFlags << " fetching address " << cr2 << endl;
            Panic();
        }

        /* Page fault handling works with address spaces, which uses mutexes */
        KernelInterruptsLock.unlock();
        try {
            CurrentProcess()->addressSpace.handlePageFault(cr2, requiredPermissions);
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
    } else {
        cout << "Unhandled x86 Exception " << registerState->intNr << " in user space" << endl;
        throw BadUserSpace(CurrentProcess());
    }
}


[[noreturn]] void _handleInterrupt(RegisterState* registerState) {
    /* noreturn function. Do not use RAII in the main function scope. It will not be properly
     * destructed at the returnToUserMode/returnInsideKernelMode/scheduleNext call. */

    bool fromUserSpace = registerState->cs == static_cast<uint64_t>(0x20|3);
    Processor *processor = CurrentProcessor();

    /* fix CurrentProcessor->interruptsLockValue first! do not print anything before! */
    if (fromUserSpace) {
        assert(processor->interruptsLockValue == 1);
    } else {
        /* x86 Exceptions can always occur, but are fatal in kernel mode. For non-exception
         * interrupts we care about the KernelInterruptsLock beeing unlocked as expected, so we
         * can return cleanly. */
        if (!X86ExceptionRegion.contains(registerState->intNr)) {
            assert(processor->interruptsLockValue == 0);
            assert(registerState->interruptsFlagSet());
        }

        /* Interrupts are disabled in Interrupt context, make our variable reflect that */
        processor->interruptsLockValue++;
    }

    if (registerState->intNr != StaticInterrupt::PAGE_FAULT) {
        cout << "Interrupt " << registerState->intNr << " on CPU " << processor->id << endl;
    }
    assert(registerState->fromSyscall == 0);

    Thread *thread = CurrentThread();
    assert(thread || (X86ExceptionRegion.contains(registerState->intNr) && !fromUserSpace));

    Exception::Action action;
    do {
        action = Exception::Action::CONTINUE;
        try {
            if (X86ExceptionRegion.contains(registerState->intNr)) {
                /* x86 Exception */
                if (fromUserSpace) {
                    handleUserException(registerState);
                } else {
                    handleKernelException(registerState);
                }
            } else if (registerState->intNr == StaticInterrupt::PIC1_SPURIOUS
                       || registerState->intNr == StaticInterrupt::PIC2_SPURIOUS) {
                cout << "Legacy Spurious IRQ!" << endl;
                //CurrentSystem.legacyPIC.handleSpuriousIRQ(registerState->intNr);
            } else if (registerState->intNr == StaticInterrupt::APIC_SPURIOUS) {
                cout << "APIC Spurious IRQ" << endl;
                /* Dispensing the spurious-interrupt vector does not affect the ISR, so the
                 * handler for this vector should return without an EOI.
                 *  - Intel Manual Vol.1 10.9 Spurious Interrupt */
                processor->endOfInterrupt();
            } else if (registerState->intNr == StaticInterrupt::IPI) {
                processor->endOfInterrupt();

                uint32_t ipis = __atomic_exchange_n(&processor->ipiFlags, 0, __ATOMIC_SEQ_CST);

                if (ipis & IPI::HaltProcessor) {
                    basicHalt();
                }
                if (ipis & IPI::CheckScheduler) {
                    processor->scheduler.checkChanges();
                }
            } else if (registerState->intNr == StaticInterrupt::TIMER) {
                processor->endOfInterrupt();
                processor->scheduler.checkChanges();
            } else if (DynamicInterruptRegion.contains(registerState->intNr)) {
                cout << "Dynamic Interrupt " << registerState->intNr << " occured" << endl;
                /* EOI first, occur may wake threads on other Processors, which may trigger the
                 * next interrupt, which we don't want to lose (edge-triggered) */
                processor->endOfInterrupt();
                InterruptManager.occur({processor->id, registerState->intNr});
            } else {
                cout << "Unknown Interrupt " << registerState->intNr << endl;
                Panic();
            }
        } catch (Exception &e) {
            action = e.handle();
        }
    } while (action == Exception::Action::RETRY);

    /* Page fault handling unlocks KernelInterruptsLock, so current Processor may have changed */
    processor = CurrentProcessor();

    /* Generic kernel work. This may be refactored to an own method. Notice that we may be running
     * with activeThread==nullptr in case of the SCHEDULE action. Should not cause problems with
     * the current kernel work. */
    KernelPageAllocator.processInvalidateQueue();
    /* Important: The following has to happen after EOI of an InvalidateQueue IPI was sent,
     * otherwise we may lose requests */
    CurrentProcessor()->invalidateQueue.localProcessing();

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

/* called from interrupt service routine */
extern "C" [[noreturn]] void handleInterrupt(RegisterState* registerState) {
    _handleInterrupt(registerState);
}

