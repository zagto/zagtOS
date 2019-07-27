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
    cout << "Unhandled Interrupt occured In Kernel Mode:" << *registerState << endl;
    Panic();
}


__attribute__((noreturn)) void Interrupts::userHandler(RegisterState *registerState) {
    Thread *currentThread = CurrentProcessor->scheduler.currentThread();
    assert(&currentThread->registerState == registerState);

    switch (registerState->intNr) {
    case SYSCALL_INTERRUPT:
        cout << "Syscall " << (size_t)registerState->syscallNr() << endl;
        if (currentThread->handleSyscall()) {
            returnToUserMode();
        }
        break;
    case 0xe:
    {
        cout << "Page Fault " << readCR2() << endl;
        static const size_t PAGING_ERROR_FLAGS{0b11111};
        static const size_t PAGING_ERROR_WRITE{0b10};
        static const size_t PAGING_ERROR_USER{0b100};

        size_t errorFlags{registerState->errorCode & PAGING_ERROR_FLAGS};
        if (errorFlags == PAGING_ERROR_USER
                || errorFlags == (PAGING_ERROR_USER | PAGING_ERROR_WRITE)) {
            if (currentThread->task->handlePageFault(UserVirtualAddress(readCR2()))) {
                returnToUserMode();
            }
        }
        break;
    }
    }

    cout << "Interrupt occured in User Mode (TODO: implement killing thread): " << *registerState << endl;
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
    cout << "Setting FS Base to " << thread->userThreadStruct().value() << endl;
    returnFromInterrupt(&CurrentProcessor->scheduler.currentThread()->registerState,
                        thread->userThreadStruct());
}


__attribute__((noreturn)) void Interrupts::handler(RegisterState *registerState) {
    cout << "Interrupt" << endl;
    assert(this == &CurrentProcessor->interrupts);

    if (registerState->cs == (0x18|3)) {
        userHandler(registerState);
    } else {
        kernelHandler(registerState);
    }
}


// called from interrupt service routine
extern "C" __attribute__((noreturn)) void handleInterrupt(RegisterState* registerState) {
    cout << "handleInterrupt" << '\n';
    CurrentProcessor->interrupts.handler(registerState);
}
