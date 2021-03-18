#include <syscalls/Time.hpp>
#include <time/Time.hpp>
#include <memory/UserSpaceObject.hpp>
#include <system/System.hpp>

void GetTime(const shared_ptr<Process> &process, uint32_t clockID, size_t resultAddress) {
    scoped_lock sl(process->pagingLock);
    assert(process->pagingLock.isLocked());
    UserSpaceObject<timespec, USOOperation::WRITE> result(resultAddress, process);
    if (!result.valid) {
        Panic(); // TODO: exception
    }

    uint64_t timerValue = readTimerValue();
    uint64_t frequency = CurrentSystem.time.timerFrequency;

    timespec &time = result.object;
    time.tv_sec = timerValue / frequency;
    time.tv_nsec = ((timerValue % frequency) * 1'000'000'000) % frequency;

    if (clockID == 0) {
        /* realtime clock */
        scoped_lock(CurrentSystem.time.offsetLock);
        if (CurrentSystem.time.offsetValid) {
            time.tv_sec += CurrentSystem.time.realTimeOffset.tv_sec;
            time.tv_nsec += CurrentSystem.time.realTimeOffset.tv_nsec;
            time.tv_sec += time.tv_nsec / 1'000'000'000;
            time.tv_nsec = time.tv_nsec % 1'000'000'000;
        } else {
            time.tv_sec = 0;
            time.tv_nsec = 0;
        }
    }
    result.writeOut();
    return;
}
