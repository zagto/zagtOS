#include <common/panic.hpp>
#include <iostream>

__attribute__((noreturn)) void Halt() {
    basicHalt();
}

__attribute__((noreturn)) void Panic() {
    cout << "Loader Panic. System will Halt";
    Halt();
}

void _Assert(bool condition, const char *message) {

    if (condition) {
        return;
    } else {
        cout << "Assertion failed";
        if (message) {
            cout << ": " << message;
        }
        cout << endl;
        Panic();
    }
}
