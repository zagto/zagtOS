#include <common/panic.hpp>
#include <log/Logger.hpp>
#ifndef ZAGTOS_LOADER
#include <interrupts/KernelInterruptsLock.hpp>
#endif

__attribute__((noreturn)) void Halt() {
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
