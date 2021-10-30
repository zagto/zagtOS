#include <common/common.hpp>
#include <system/System.hpp>
#include <interrupts/Interrupts.hpp>
#include <interrupts/ContextSwitch.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <system/Processor.hpp>
#include <processes/KernelThreadEntry.hpp>


InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;


[[noreturn]] void handleKernelException(RegisterState *registerState) {
    switch (registerState->intNr) {
    default:
        cout << "x86 Exception occured In Kernel Mode:" << *registerState
             << " on Processor " << CurrentProcessor->id << endl;
        Panic();
    }
}

void dealWithException(Status status) {
    Thread *activeThread = CurrentProcessor->scheduler.activeThread();
    if (status == Status::DiscardStateAndSchedule()) {
        /* make sure status holds no dynamic allocations because destructor is never called */
        status = {};
        CurrentProcessor->scheduler.scheduleNext();
    } else if (status == Status::BadUserSpace()) {
        Status status2 = activeThread->process->crash("BadUserSpace Exception");
        if (!status2) {
            assert(status2 == Status::ThreadKilled());
            /* make sure status holds no dynamic allocations because destructor is never called */
            status = {};
            dealWithException(move(status2));
        }
    } else if (status == Status::OutOfKernelHeap()) {
        cout << "TODO: deal with OutOfKernelHeap" << endl;
        Panic();
    } else if (status == Status::OutOfMemory()) {
        cout << "TODO: deal with OutOfMemory" << endl;
        Panic();
    } else if (status == Status::ThreadKilled()) {
        CurrentProcessor->scheduler.lock.lock();
        CurrentProcessor->scheduler.removeActiveThread();
        CurrentProcessor->scheduler.scheduleNext();
        cout << "TODO: deal with ThreadKilled" << endl;
        Panic();
    } else {
        cout << "Unknown Exception: " << status << endl;
        Panic();
    }
}

Status handleUserException(RegisterState *registerState) {
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
        Status status = CurrentProcess()->addressSpace.handlePageFault(cr2, requiredPermissions);
        KernelInterruptsLock.lock();

        return status;
    } else {
        cout << "Unhandled x86 Exception " << registerState->intNr << " in user space" << endl;
        return Status::BadUserSpace();
    }
}


[[noreturn]] void _handleInterrupt(RegisterState* registerState) {
    bool fromUserSpace = registerState->cs == static_cast<uint64_t>(0x20|3);

    /* fix CurrentProcessor->interruptsLockValue first! do not print anything before! */
    if (fromUserSpace) {
        assert(CurrentProcessor->interruptsLockValue == 1);
    } else {
        /* x86 Exceptions can always occur, but are fatal in kernel mode. For non-exception
         * interrupts we care about the KernelInterruptsLock beeing unlocked as expected, so we
         * can return cleanly. */
        if (!X86ExceptionRegion.contains(registerState->intNr)) {
            assert(CurrentProcessor->interruptsLockValue == 0);
            assert(registerState->interruptsFlagSet());
        }
        /* Interrupts are disabled in Interrupt context, make our variable reflect that */
        CurrentProcessor->interruptsLockValue++;
    }

    if (registerState->intNr != StaticInterrupt::PAGE_FAULT) {
        cout << "Interrupt " << registerState->intNr << " on CPU " << CurrentProcessor->id << endl;
    }
    assert(registerState->fromSyscall == 0);

    Thread *thread = CurrentThread();
    assert(thread);

    Status status = Status::OK();

    if (X86ExceptionRegion.contains(registerState->intNr)) {
        /* x86 Exception */
        if (fromUserSpace) {
            status = handleUserException(registerState);
        } else {
            handleKernelException(registerState);
        }
    } else if (registerState->intNr == StaticInterrupt::PIC1_SPURIOUS
               || registerState->intNr == StaticInterrupt::PIC2_SPURIOUS) {
        cout << "Legacy Spurious IRQ!" << endl;
        //CurrentSystem.legacyPIC.handleSpuriousIRQ(registerState->intNr);
    } else if (registerState->intNr == StaticInterrupt::APIC_SPURIOUS) {
        cout << "APIC Spurious IRQ" << endl;
        CurrentProcessor->endOfInterrupt();
    } else if (registerState->intNr == StaticInterrupt::IPI) {
        CurrentProcessor->endOfInterrupt();

        uint32_t ipis = __atomic_exchange_n(&CurrentProcessor->ipiFlags, 0, __ATOMIC_SEQ_CST);

        if (ipis & IPI::CheckScheduler) {
            status = CurrentProcessor->scheduler.checkChanges();
            if (status) {
                cout << "Info: CheckProcessor IPI did not lead to change." << endl;
            }
        }
    } else {
        cout << "Unknown Interrupt " << registerState->intNr << endl;
        Panic();
    }

    if (status) {
        /* Generic kernel work. This may be refactored to an own method */
        KernelPageAllocator.processInvalidateQueue();
        /* Important: The following has to happen after EOI of an InvalidateQueue IPI was sent,
         * otherwise we may lose requests */
        CurrentProcessor->invalidateQueue.localProcessing();
    }

    assert(KernelInterruptsLock.isLocked());

    /* It is important to set these down here instead of at the beginning. Code like Page Fault
     * handling enables Interrupts, which could override these variables. */
    if (fromUserSpace) {
        thread->setKernelEntry(UserReturnEntry);
    } else {
        thread->setKernelEntry(InKernelReturnEntry, registerState);
    }

    if (status) {
        assert(CurrentProcessor->interruptsLockValue == 1);
        if (fromUserSpace) {
            CurrentProcessor->returnToUserMode();
        } else {
            CurrentProcessor->interruptsLockValue--;
            /* We are returning from an interrupt, which means the interrupted code had interrupts
             * enabled. */
            CurrentProcessor->returnInsideKernelMode(registerState);
        }
    } else {
        dealWithException(status);
        cout << "TODO: may there be ways to continue here in the future" << endl;
        Panic();
    }
}

/* called from interrupt service routine */
extern "C" __attribute__((noreturn)) void handleInterrupt(RegisterState* registerState) {
    _handleInterrupt(registerState);
}
