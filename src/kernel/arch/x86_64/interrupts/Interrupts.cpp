#include <common/common.hpp>
#include <system/System.hpp>
#include <interrupts/Interrupts.hpp>
#include <interrupts/ContextSwitch.hpp>
#include <interrupts/util.hpp>
#include <tasks/Task.hpp>


InterruptDescriptorTable INTERRUPT_DESCRIPTOR_TABLE;


Interrupts::Interrupts() :
        globalDescriptorTable(&taskStateSegment),
        globalDescriptorTableRecord(&globalDescriptorTable),
        interruptDescriptorTableRecord(&INTERRUPT_DESCRIPTOR_TABLE) {

    globalDescriptorTableRecord.load();
    interruptDescriptorTableRecord.load();
    loadTaskStateSegment();
}


__attribute__((noreturn)) void Interrupts::kernelHandler(RegisterState *registerState) {
    Log << "Unhandled Interrupt occured In Kernel Mode:" << *registerState << EndLine;
    Panic();
}


__attribute__((noreturn)) void Interrupts::userHandler(RegisterState *registerState) {
    Thread *currentThread = CurrentProcessor->scheduler.currentThread();
    Assert(&currentThread->registerState == registerState);

    switch (registerState->intNr) {
    case SYSCALL_INTERRUPT:
        Log << "Syscall " << (usize)registerState->syscallNr() << EndLine;
        if (currentThread->handleSyscall()) {
            returnToUserMode();
        }
        break;
    case 0xe:
    {
        Log << "Page Fault " << readCR2() << EndLine;
        static const usize PAGING_ERROR_FLAGS{0b11111};
        static const usize PAGING_ERROR_WRITE{0b10};
        static const usize PAGING_ERROR_USER{0b100};

        usize errorFlags{registerState->errorCode & PAGING_ERROR_FLAGS};
        if (errorFlags == PAGING_ERROR_USER
                || errorFlags == (PAGING_ERROR_USER | PAGING_ERROR_WRITE)) {
            if (currentThread->task->handlePageFault(UserVirtualAddress(readCR2()))) {
                returnToUserMode();
            }
        }
        break;
    }
    }

    Log << "Interrupt occured in User Mode (TODO: implement killing thread): " << *registerState << EndLine;
    Panic();

}


__attribute__((noreturn)) void Interrupts::returnToUserMode() {
    Thread *thread = CurrentProcessor->scheduler.currentThread();
    // Idle thread does not have a task
    if (thread->task) {
        thread->task->activate();
    }
    globalDescriptorTable.resetTaskStateSegment();
    taskStateSegment.update(thread);
    Log << "Setting FS Base to " << thread->userThreadStruct().value() << EndLine;
    returnFromInterrupt(&CurrentProcessor->scheduler.currentThread()->registerState,
                        thread->userThreadStruct());
}


__attribute__((noreturn)) void Interrupts::handler(RegisterState *registerState) {
    Log << "Interrupt" << EndLine;
    Assert(this == &CurrentProcessor->interrupts);

    if (registerState->cs == (0x18|3)) {
        userHandler(registerState);
    } else {
        kernelHandler(registerState);
    }
}


// called from interrupt service routine
extern "C" __attribute__((noreturn)) void handleInterrupt(RegisterState* registerState) {
    Log << "handleInterrupt" << '\n';
    CurrentProcessor->interrupts.handler(registerState);
}
