#include <common/common.hpp>
#include <lib/atexit.hpp>

/* very simplified implementation of __cxa_atexit that only supports one call */
static void (*atExitFunction) (void *);
static void *atExitArgument;
static bool atExitRegistered{false};

extern "C"
int __cxa_atexit(void (*function) (void *), void *argument, void *) {
    assert(!atExitRegistered);
    atExitRegistered = true;
    atExitFunction = function;
    atExitArgument = argument;
    return 0;
}

void CallAtExitFunction() {
    assert(atExitRegistered);
    atExitFunction(atExitArgument);
}

