#include <common/panic.hpp>
#include <log/Logger.hpp>
#ifndef ZAGTOS_LOADER
#include <interrupts/KernelInterruptsLock.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
#endif

__attribute__((noreturn)) void Halt() {
#ifndef ZAGTOS_LOADER
    if (ProcessorsInitialized) {
        for (size_t processorID = 0; processorID < CurrentSystem.numProcessors; processorID++) {
            if (processorID != CurrentProcessor()->id) {
                Processors[processorID].sendIPI(IPI::HaltProcessor);
            }
        }
    }
#endif
    basicHalt();
}

__attribute__((noreturn)) void Panic() {
#ifdef ZAGTOS_LOADER
    cout << "Loader Panic. System will Halt";
#else
    /* ensure Interrupts disabled */
    KernelInterruptsLock.lock();

    cout << "Kernel Panic. System will Halt";
#endif
    Halt();
}

void _Assert(bool condition, const char *message) {

    if (condition) {
        return;
    } else {
#ifndef ZAGTOS_LOADER
        /* ensure Interrupts disabled */
        KernelInterruptsLock.lock();
#endif

        cout << "Assertion failed";
        if (message) {
            cout << ": " << message;
        }
        cout << endl;
        Panic();
    }
}
