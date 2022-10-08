#include <common/panic.hpp>
#include <interrupts/KernelInterruptsLock.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
#include <iostream>

__attribute__((noreturn)) void Halt() {
    if (ProcessorsInitialized) {
        for (size_t processorID = 0; processorID < CurrentSystem.numProcessors; processorID++) {
            if (processorID != CurrentProcessor()->id) {
                Processors[processorID].sendIPI(IPI::HaltProcessor);
            }
        }
    }
    basicHalt();
}

__attribute__((noreturn)) void Panic() {
    /* ensure Interrupts disabled */
    KernelInterruptsLock.lock();

    cout << "Kernel Panic. System will Halt" << endl;
    Halt();
}

void _Assert(bool condition, const char *message) {

    if (condition) {
        return;
    } else {
        /* ensure Interrupts disabled */
        KernelInterruptsLock.lock();

        cout << "Assertion failed";
        if (message) {
            cout << ": " << message;
        }
        cout << endl;
        Panic();
    }
}
