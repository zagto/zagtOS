#include <common/common.hpp>
#include <time/Time.hpp>
#include <syscalls/NanoSleep.hpp>
#include <syscalls/UserSpaceObject.hpp>
#include <system/Processor.hpp>
#include <optional>

optional<uint64_t> convertTimespec(timespec ts) {
    constexpr uint64_t nsec_to_sec = 1'000'000'000;
    if (ts.tv_nsec >= nsec_to_sec) {
        return {};
    }
    if (ts.tv_sec > 0xffff'ffff'ffff'ffff / nsec_to_sec) {
        /* value too large to represent as a single uint64_t */
        return {};
    }
    return ts.tv_nsec + ts.tv_sec * nsec_to_sec;
}

size_t NanoSleep(const shared_ptr<Process> &process,
                 size_t flags,
                 size_t clockID,
                 size_t timespecAddress,
                 size_t,
                 size_t) {
    //constexpr size_t TIMER_ABSTIME = 1;

    if (clockID >= ClockID::COUNT) {
        cout << "NanoSleep: unknown clock ID: " << clockID << endl;
        throw BadUserSpace(process);
    }

    if (clockID != ClockID::MONOTONIC) {
        cout << "Not implemented: clock id " << clockID << " in NanoSleep" << endl;
        assert(false);
    }

    UserSpaceObject<timespec, USOOperation::READ> uso(timespecAddress);


    if (flags == 0) {
        uint64_t now = readTimerValue();
        optional<uint64_t> interval = convertTimespec(uso.object);
        if (!interval) {
            cout << "NanoSleep: timespec value too large to be represented in a 64-but nanosecond "
                 << "value or invalid" << endl;
            throw BadUserSpace(process);
        }
        if (*interval + now < now) {
            cout << "NanoSleep: interval too large" << endl;
            throw BadUserSpace(process);
        }

        CurrentProcessor()->scheduler.setTimer(static_cast<ClockID>(clockID), now + *interval);
    } else {
        cout << "NanoSleep: unknown flags: " << flags << endl;
        throw BadUserSpace(process);
    }
    return 0;
}
