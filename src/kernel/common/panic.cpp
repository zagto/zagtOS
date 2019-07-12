#include <common/common.hpp>


__attribute__((noreturn)) void Halt() {
    basicHalt();
}

__attribute__((noreturn)) void Panic() {
    Log << "Kernel Panic. System will Halt";
    Halt();
}

void _Assert(bool condition, const char *message) {

    if (condition) {
        return;
    } else {
        Log << "Assertion failed";
        if (message) {
            Log << ": " << message;
        }
        Log << EndLine;
        Panic();
    }
}
