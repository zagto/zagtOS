#include <common/common.hpp>
#include <system/System.hpp>
#include <interrupts/Interrupts.hpp>
#include <interrupts/ContextSwitch.hpp>
#include <interrupts/util.hpp>
#include <processes/Process.hpp>
#include <common/ModelSpecificRegister.hpp>


InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;


Interrupts::Interrupts(bool bootProcessor) :
        globalDescriptorTable(&taskStateSegment),
        globalDescriptorTableRecord(&globalDescriptorTable),
        interruptDescriptorTableRecord(&INTERRUPT_DESCRIPTOR_TABLE),
        legacyPIC(bootProcessor),
        localAPIC(readModelSpecificRegister(MSR::IA32_APIC_BASE)) {

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


__attribute__((noreturn)) void Interrupts::userHandler(RegisterState *registerState) {
    bool handled = false;

    {
        Thread *activeThread = CurrentProcessor->scheduler.activeThread();
        assert(&activeThread->registerState == registerState);

        switch (registerState->intNr) {
        case PIC1_SPURIOUS_IRQ:
        case PIC2_SPURIOUS_IRQ:
            cout << "Spurious IRQ in user mode!" << endl;
            legacyPIC.handleSpuriousIRQ(registerState->intNr);
            handled = true;
            break;
        case 0xe:
        {
            static const size_t PAGING_ERROR_FLAGS{0b11111};
            static const size_t PAGING_ERROR_WRITE{0b10};
            static const size_t PAGING_ERROR_USER{0b100};

            size_t cr2 = readCR2();

            size_t errorFlags{registerState->errorCode & PAGING_ERROR_FLAGS};
            if (!(errorFlags & PAGING_ERROR_USER)) {
                cout << "userHandler called for non user page fault" << endl;
                Panic();
            }
            if (errorFlags == PAGING_ERROR_USER
                    || errorFlags == (PAGING_ERROR_USER | PAGING_ERROR_WRITE)) {
                if (activeThread->process->handlePageFault(UserVirtualAddress(cr2))) {
                    handled = true;
                }
            }
            assert(&activeThread->registerState == registerState);
            if (!handled) {
                cout << "Unhandled Page Fault for in User Mode address: " << cr2 << endl;
                activeThread->process->crash("Unhandled Page Fault", activeThread);
            }
            break;
        }
        case 0xd:
            activeThread->process->crash("General Protection Fault", activeThread);
            break;
        }
    }

    if (handled) {
        returnToUserMode();
    }


    cout << "Interrupt occured in User Mode (TODO: implement killing thread): " << *registerState << endl;
    Panic();
}


__attribute__((noreturn)) void Interrupts::returnToUserMode() {
    Thread *thread = CurrentProcessor->scheduler.activeThread();
    // Idle thread does not have a process
    if (thread->process) {
        thread->process->pagingContext->activate();
    }
    globalDescriptorTable.resetTaskStateSegment();
    taskStateSegment.update(thread);
    returnFromInterrupt(&thread->registerState, thread->threadLocalStorage());
}


__attribute__((noreturn)) void Interrupts::handler(RegisterState *registerState) {
    assert(this == &CurrentProcessor->interrupts);

    if (registerState->cs == static_cast<uint64_t>(0x18u|3u)) {
        userHandler(registerState);
    } else {
        kernelHandler(registerState);
    }
}


/* called from interrupt service routine */
extern "C" __attribute__((noreturn)) void handleInterrupt(RegisterState* registerState) {
    assert(registerState->cs < 0x20);
    CurrentProcessor->interrupts.handler(registerState);
}

void Interrupts::wakeSecondaryProcessor(size_t hardwareID) {
    CurrentSystem.processorsLock.lock();
    localAPIC.sendInit(static_cast<uint32_t>(hardwareID));
    localAPIC.timer.delayMilliseconds(10);
    localAPIC.sendStartup(static_cast<uint32_t>(hardwareID), PhysicalAddress(CurrentSystem.secondaryProcessorEntry));
}

void Interrupts::setupSyscalls() {
    cout << "registering syscall entry " << reinterpret_cast<uint64_t>(&syscallEntry) << endl;
    writeModelSpecificRegister(MSR::LSTAR, reinterpret_cast<uint64_t>(&syscallEntry));
    writeModelSpecificRegister(MSR::STAR, (static_cast<uint64_t>(0x08) << 32) | (static_cast<uint64_t>(0x08) << 48));
    writeModelSpecificRegister(MSR::SFMASK, RegisterState::FLAG_INTERRUPTS | RegisterState::FLAG_USER_IOPL);

}
