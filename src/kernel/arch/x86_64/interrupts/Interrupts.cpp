#include <common/common.hpp>
#include <system/System.hpp>
#include <interrupts/Interrupts.hpp>
#include <interrupts/ContextSwitch.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <common/ModelSpecificRegister.hpp>
#include <system/Processor.hpp>


InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;


__attribute__((noreturn)) void kernelHandler(RegisterState *registerState) {
    switch (registerState->intNr) {
    default:
        cout << "Unhandled x86 Exception occured In Kernel Mode:" << *registerState
             << " on Processor " << CurrentProcessor->id << endl;
        Panic();
    }
}

void dealWithException(Status status) {
    Thread *activeThread = CurrentProcessor->scheduler.activeThread();
    if (status == Status::BadUserSpace()) {
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
        cout << "TODO: deal with ThreadKilled" << endl;
        Panic();
    } else {
        cout << "Unknown Exception" << endl;
        Panic();
    }
}

__attribute__((noreturn)) void userHandler(RegisterState *registerState) {
    {
        Thread *activeThread = CurrentProcessor->scheduler.activeThread();
        assert(&activeThread->registerState == registerState);

        switch (registerState->intNr) {
        case PIC1_SPURIOUS_IRQ:
        case PIC2_SPURIOUS_IRQ:
            break;
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
                status = activeThread->process->addressSpace.handlePageFault(cr2,
                                                                             requiredPermissions);
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
            cout << "x86 Exception " << registerState->intNr << endl;
            Panic();
        }
    }

    CurrentProcessor->returnToUserMode();
}


__attribute__((noreturn)) void handler(RegisterState *registerState) {
    assert(registerState->cs == (0x20|3) || registerState->cs == 0x8);

    if (registerState->cs == static_cast<uint64_t>(0x20|3)) {
        userHandler(registerState);
    } else {
        kernelHandler(registerState);
    }
}


__attribute__((noreturn)) void _handleInterrupt(RegisterState* registerState) {
    if (registerState->intNr < 0x20) {
        /* x86 Exception */
        if (registerState->cs == static_cast<uint64_t>(0x20|3)) {
            userHandler(registerState);
        } else {
            kernelHandler(registerState);
        }
    } else if (registerState->intNr == PIC1_SPURIOUS_IRQ
               || registerState->intNr == PIC2_SPURIOUS_IRQ) {
        cout << "Spurious IRQ!" << endl;
        CurrentSystem.legacyPIC.handleSpuriousIRQ(registerState->intNr);
        CurrentProcessor->returnToUserMode();
    } else {
        cout << "Unknown Interrupt " << registerState->intNr << endl;
        Panic();
    }
}

/* called from interrupt service routine */
extern "C" __attribute__((noreturn)) void handleInterrupt(RegisterState* registerState) {
    _handleInterrupt(registerState);
}
