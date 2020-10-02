#include <common/panic.hpp>
#include <log/Logger.hpp>

__attribute__((noreturn)) void Halt() {
    basicHalt();
}

__attribute__((noreturn)) void Panic() {
    cout << "Kernel Panic. System will Halt";
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
