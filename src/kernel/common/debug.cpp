#include <common/common.hpp>
#include <common/debug.hpp>
#include <processes/Process.hpp>

__attribute__((noreturn)) void enterDebugger(shared_ptr<Process>) {
    cout << "Entering Debugger. Waiting for GDB on Serial Port..." << endl;
    while (true) {
        char c = cout.read();
        cout << "got character: " << c << endl;
    }
}
