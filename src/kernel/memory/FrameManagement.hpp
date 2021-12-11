#pragma once
#include <memory/FrameStack.hpp>
#include <setup/HandOverState.hpp>
#include <mutex>
#include <optional>

namespace frameManagement {

typedef size_t ZoneID;
static constexpr size_t DEFAULT_ZONE_ID = hos_v1::DMAZone::COUNT - 1;

class Management {
private:
    static constexpr size_t NUM_ZONES = hos_v1::DMAZone::COUNT;

    SpinLock lock;
    FrameStack usedFrameStack[NUM_ZONES];
    FrameStack freshFrameStack[NUM_ZONES];

    void recycleFrame(ZoneID zoneID) noexcept;

public:
    Management() noexcept;
    PhysicalAddress get(ZoneID zoneID);
    void put(PhysicalAddress frame) noexcept;
};

extern Management FrameManagement;
}

using frameManagement::FrameManagement;
