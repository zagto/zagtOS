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


[[noreturn]] void kernelHandler(RegisterState *registerState) {
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
        CurrentProcessor->scheduler.scheduleNext();
    } else if (status == Status::BadUserSpace()) {
        Status status2 = activeThread->process->crash("BadUserSpace Exception", activeThread);
        assert(status2 == Status::ThreadKilled());
        dealWithException(status2);
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

[[noreturn]] void userHandler(RegisterState *registerState) {
    {
        assert(CurrentThread()->kernelStack->userRegisterState() == registerState);

        switch (registerState->intNr) {
        case 0xe:
        {
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
            Status status;
            do {
                /* Page fault handling works with address spaces, which uses mutexes */
                KernelInterruptsLock.unlock();
                status = CurrentThread()->process->addressSpace.handlePageFault(
                            cr2, requiredPermissions);
                KernelInterruptsLock.lock();
                if (!status) {
                    dealWithException(status);
                }
            } while(!status);
            break;
        }
        case 0xd:
            cout << "General Protection Fault in user space" << endl;
            dealWithException(Status::BadUserSpace());

            cout << "should not reach here" << endl;
            Panic();
        default:
            cout << "Unhandled x86 Exception " << registerState->intNr << " in user space" << endl;
            Panic();
        }
    }

    CurrentProcessor->returnToUserMode();
}

static Region X86ExceptionRegion{0x00, 0x20};
static Region LegacySpuriousIRQRegion{0x20, 0x02};

[[noreturn]] void _handleInterrupt(RegisterState* registerState) {
    if (registerState->intNr != 0xe) {
        cout << "Interrupt " << registerState->intNr << " on CPU " << CurrentProcessor->id << endl;
    }

    bool fromUserSpace = registerState->cs == static_cast<uint64_t>(0x20|3);

    if (fromUserSpace) {
        assert(CurrentProcessor->interruptsLockValue == 1);
    } else {
        /* x86 Exceptions can always occur, but are fatal in kernel mode. For non-exception
         * interrupts we care about the KernelInterruptsLock beeing unlocked as expected, so we
         * can return cleanly. */
        if (X86ExceptionRegion.contains(registerState->intNr)) {
            assert(CurrentProcessor->interruptsLockValue == 0);
            assert(registerState->interruptsFlagSet());
        }
        /* Interrupts are disabled in Interrupt context, make our variable reflect that */
        CurrentProcessor->interruptsLockValue++;
    }

    Thread *thread = CurrentThread();
    assert(thread);

    Status status;

    KernelPageAllocator.processInvalidateQueue();
    CurrentProcessor->invalidateQueue.localProcessing();

    if (X86ExceptionRegion.contains(registerState->intNr)) {
        /* x86 Exception */
        if (fromUserSpace) {
            userHandler(registerState);
        } else {
            kernelHandler(registerState);
        }
    } else if (LegacySpuriousIRQRegion.contains(registerState->intNr)) {
        cout << "Spurious IRQ!" << endl;
        //CurrentSystem.legacyPIC.handleSpuriousIRQ(registerState->intNr);
        status = Status::OK();
    } else if (registerState->intNr == 0x40) {
        /* check scheduler IPI */
        status = CurrentProcessor->scheduler.checkChanges();
        if (status) {
            cout << "Info: CheckProcessor IPI did not lead to change." << endl;
        }
    } else if (registerState->intNr == 0x41) {
        /* Process Invalidate Queue - we allways to this */
        status = Status::OK();
    } else {
        cout << "Unknown Interrupt " << registerState->intNr << endl;
        Panic();
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
        if (fromUserSpace) {
            CurrentProcessor->returnToUserMode();
        } else {
            CurrentProcessor->interruptsLockValue--;
            /* We are returning from an interrupt, which means the interrupted code had interrupts
             * enabled. */
            assert(CurrentProcessor->interruptsLockValue == 0);
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
