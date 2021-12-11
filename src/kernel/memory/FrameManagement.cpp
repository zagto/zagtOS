#include <memory/FrameManagement.hpp>
#include <paging/PagingContext.hpp>
#include <system/System.hpp>
#include <lib/Exception.hpp>

namespace frameManagement {

Management::Management() noexcept {
    for (size_t stackIndex = 0; stackIndex < NUM_ZONES; stackIndex++) {
        usedFrameStack[stackIndex] = _HandOverSystem->usedFrameStack[stackIndex];
        freshFrameStack[stackIndex] = _HandOverSystem->freshFrameStack[stackIndex];
    }
}

PhysicalAddress Management::get(ZoneID zoneID) {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(lock);
    assert(zoneID < NUM_ZONES);

    for (int stackIndex = zoneID; stackIndex >= 0; stackIndex--) {
        if (!freshFrameStack[stackIndex].isEmpty() || !usedFrameStack[stackIndex].isEmpty()) {
            if (freshFrameStack[stackIndex].isEmpty()) {
                recycleFrame(stackIndex);
            }
            return freshFrameStack[stackIndex].pop();
        }
    }
    throw OutOfMemory();
}

void Management::put(PhysicalAddress frame) noexcept {
    scoped_lock lg1(KernelInterruptsLock);
    scoped_lock lg(lock);
    for (size_t stackIndex = 0; stackIndex < NUM_ZONES; stackIndex++) {
        if (frame.value() <= hos_v1::DMAZoneMax[stackIndex]) {
            usedFrameStack[stackIndex].push(frame);
            return;
        }
    }
    cout << "should not reach here" << endl;
    Panic();
}

void Management::recycleFrame(ZoneID zoneID) noexcept {
    assert(lock.isLocked());
    PhysicalAddress frame = usedFrameStack[zoneID].pop();
    memset(frame.identityMapped().asPointer<uint8_t>(), 0, PAGE_SIZE);
    freshFrameStack[zoneID].push(frame);
}

}
