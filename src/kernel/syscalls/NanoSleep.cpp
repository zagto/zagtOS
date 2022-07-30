#include <common/common.hpp>
#include <syscalls/NanoSleep.hpp>

size_t NanoSleep(const shared_ptr<Process> &process,
                 size_t flags,
                 size_t clockID,
                 size_t timpespec,
                 size_t,
                 size_t) {
    constexpr size_t TIMER_ABSTIME = 1;
    constexpr size_t CLOCK_REALTIME = 0;
    constexpr size_t CLOCK_MONOTONIC = 1;

    uint64_t now = readTimerValue();

    if (clockID != CLOCK_MONOTONIC) {
        cout << "Not implemented: clock id " << clockID << " in NanoSleep" << endl;
        assert(false);
    }

    if (!(flags & TIMER_ABSTIME)) {
        cout << "Not implemented: TIMER_ABSTIME" << endl;
        assert(false);
    }
}
