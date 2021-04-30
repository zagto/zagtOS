#pragma once
#include <memory/FrameStack.hpp>
#include <setup/HandOverState.hpp>
#include <mutex>
#include <optional>
#include <lib/Status.hpp>

namespace frameManagement {

typedef size_t ZoneID;

class Management {
private:
    static const size_t NUM_ZONES = hos_v1::DMAZone::COUNT;

    mutex lock;
    FrameStack usedFrameStack[NUM_ZONES];
    FrameStack freshFrameStack[NUM_ZONES];

    void recycleFrame(ZoneID zoneID);

public:
    Management();
    Result<PhysicalAddress> get(ZoneID zoneID = NUM_ZONES - 1);
    void put(PhysicalAddress frame);
    optional<ZoneID> zoneForAddressCeiling(KernelVirtualAddress ceiling);
};

extern Management FrameManagement;
}

using frameManagement::FrameManagement;
