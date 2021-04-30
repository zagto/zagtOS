#include <common/common.hpp>
#include <lib/atexit.hpp>

/* implementation of __cxa_atexit */

struct AtExitEntry {
    void (*function) (void *);
    void *argument;
};

static const size_t maxAtExits = 16;
static size_t numAtExits = 0;
static AtExitEntry atExitData[maxAtExits];

extern "C"
int __cxa_atexit(void (*function) (void *), void *argument, void *) {
    assert(numAtExits < maxAtExits);
    atExitData[numAtExits] = {
        function,
        argument,
    };
    numAtExits++;
    return 0;
}

void CallAtExitFunctions() {
    for (size_t index = 0; index < numAtExits; index++) {
        atExitData[index].function(atExitData[index].argument);
    }
}

