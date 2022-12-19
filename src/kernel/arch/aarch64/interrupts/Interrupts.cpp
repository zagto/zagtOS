#include <interrupts/Interrupts.hpp>
#include <interrupts/RegisterState.hpp>
#include <iostream>

[[noreturn]] void _handleInterrupt(RegisterState* registerState) {
    /* noreturn function. Do not use RAII in the main function scope. It will not be properly
     * destructed at the returnToUserMode/returnInsideKernelMode/scheduleNext call. */

    cout << "Interrupt occured: " << registerState << endl;
    Panic();
}


/* called from exception vector */
extern "C" [[noreturn]] void handleInterrupt(RegisterState* registerState) {
    _handleInterrupt(registerState);
}
