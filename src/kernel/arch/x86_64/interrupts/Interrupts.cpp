#include <common/common.hpp>
#include <system/System.hpp>
#include <interrupts/Interrupts.hpp>
#include <interrupts/ContextSwitch.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <common/ModelSpecificRegister.hpp>


InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;


Interrupts::Interrupts(bool bootProcessor, Status &status) :
        globalDescriptorTable(&taskStateSegment),
        globalDescriptorTableRecord(&globalDescriptorTable),
        interruptDescriptorTableRecord(&INTERRUPT_DESCRIPTOR_TABLE),
        taskStateSegment(status),
        legacyPIC(bootProcessor),
        localAPIC(readModelSpecificRegister(MSR::IA32_APIC_BASE), status) {

    if (!status) {
        return;
    }
    globalDescriptorTableRecord.load();
    interruptDescriptorTableRecord.load();
    loadTaskStateSegment();
    setupSyscalls();
}


__attribute__((noreturn)) void Interrupts::kernelHandler(RegisterState *registerState) {
    switch (registerState->intNr) {
    case PIC1_SPURIOUS_IRQ:
    case PIC2_SPURIOUS_IRQ:
        cout << "Spurious IRQ in kernel mode!" << endl;
        legacyPIC.handleSpuriousIRQ(registerState->intNr);
        returnFromInterrupt(registerState, {});
    default:
        cout << "Unhandled Interrupt occured In Kernel Mode:" << *registerState << endl;
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

__attribute__((noreturn)) void Interrupts::userHandler(RegisterState *registerState) {
    {
        Thread *activeThread = CurrentProcessor->scheduler.activeThread();
        assert(&activeThread->registerState == registerState);

        switch (registerState->intNr) {
        case PIC1_SPURIOUS_IRQ:
        case PIC2_SPURIOUS_IRQ:
            cout << "Spurious IRQ in user mode!" << endl;
            legacyPIC.handleSpuriousIRQ(registerState->intNr);
            break;
        case 0xe:
        {
            static const size_t PAGING_ERROR_FLAGS{0b11111};
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
            switch (errorFlags) {
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
                cout << "Unexpected Page Fault Flags: " << errorFlags << endl;
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
            cout << "Unexpected Interrupt " << registerState->intNr << endl;
            Panic();
        }
    }

    returnToUserMode();
}


__attribute__((noreturn)) void Interrupts::returnToUserMode() {
    Thread *thread = CurrentProcessor->scheduler.activeThread();
    // Idle thread does not have a process
    if (thread->process) {
        thread->process->addressSpace.activate();
    }
    globalDescriptorTable.resetTaskStateSegment();
    taskStateSegment.update(thread);
    returnFromInterrupt(&thread->registerState, thread->threadLocalStorage());
}


__attribute__((noreturn)) void Interrupts::handler(RegisterState *registerState) {
    assert(this == &CurrentProcessor->interrupts);
    assert(registerState->cs == (0x20|3) || registerState->cs == 0x8);

    if (registerState->cs == static_cast<uint64_t>(0x20|3)) {
        userHandler(registerState);
    } else {
        kernelHandler(registerState);
    }
}


/* called from interrupt service routine */
extern "C" __attribute__((noreturn)) void handleInterrupt(RegisterState* registerState) {
    CurrentProcessor->interrupts.handler(registerState);
}

void Interrupts::setupSyscalls() {
    cout << "registering syscall entry " << reinterpret_cast<uint64_t>(&syscallEntry) << endl;
    writeModelSpecificRegister(MSR::LSTAR, reinterpret_cast<uint64_t>(&syscallEntry));
    writeModelSpecificRegister(MSR::STAR, (static_cast<uint64_t>(0x08) << 32) | (static_cast<uint64_t>(0x10) << 48));
    writeModelSpecificRegister(MSR::SFMASK, RegisterState::FLAG_INTERRUPTS | RegisterState::FLAG_USER_IOPL);

}

