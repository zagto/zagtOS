#include <syscalls/GetTime.hpp>
#include <syscalls/UserSpaceObject.hpp>
#include <system/System.hpp>

size_t GetTime(const shared_ptr<Process> &process,
                       size_t clockID,
                       size_t resultAddress,
                       size_t,
                       size_t,
                       size_t) {
    UserSpaceObject<timespec, USOOperation::WRITE> result(resultAddress);

    if (clockID > ClockID::COUNT) {
        cout << "GetTime: unknown clock ID " << clockID << endl;
        throw BadUserSpace(process);
    }
    uint64_t nanoseconds = CurrentSystem.time.getClockValue(clockID);

    timespec &time = result.object;
    time.tv_sec = nanoseconds / 1'000'000'000;
    time.tv_nsec = nanoseconds % 1'000'000'000;

    result.writeOut();
    return 0;
}


