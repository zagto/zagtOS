#include <common/common.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
#include <memory/TLBContext.hpp>

CommonSystem::CommonSystem(const hos_v1::System &handOver):
    nextFutexFrameID{handOver.nextFutexFrameID},
    time{handOver.timerFrequency},
    kernelOnlyPagingContext(handOver.handOverPagingContext),
    numProcessors{handOver.numProcessors},
    firmwareRoot{_HandOverSystem->firmwareRoot} {
    // TODO: initialize FutexManager?
    cout << "CommonSystem()" << endl;

}

void CommonSystem::initProcessorsAndTLB() {
    TLBContexts = new TLBContext[numProcessors * tlbContextsPerProcessor];
    Processors = new Processor[numProcessors];
}

FutexFrameID CommonSystem::getNewFutexFrameID() noexcept {
    FutexFrameID result = __atomic_fetch_add(&nextFutexFrameID, 1, __ATOMIC_SEQ_CST);

    /* These IDs are combined with the low bits of the address to create the individual Futex IDs
     * inside the frame */
    assert(((nextFutexFrameID << PAGE_SHIFT) >> PAGE_SHIFT) == nextFutexFrameID);

    return result;
}

uint64_t CommonSystem::getNextTLBTimetamp() noexcept {
    return __atomic_fetch_add(&nextTLBTimestamp, 1, __ATOMIC_SEQ_CST);
}
