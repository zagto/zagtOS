#include <common/common.hpp>
#include <system/System.hpp>
#include <memory/TLBContext.hpp>

CommonSystem::CommonSystem(const hos_v1::System &handOver, Status &status):
    nextFutexFrameID{handOver.nextFutexFrameID},
    kernelOnlyPagingContext(handOver.handOverPagingContext, status),
    futexManager(status),
    numProcessors{handOver.numProcessors} {}

Status CommonSystem::initProcessorsAndTLB() {
    cout << "Initializing first processor..." << endl;
    Result<Processor *> processors = make_raw_array_counter<Processor>(numProcessors);
    if (!processors) {
        return processors.status();
    }
    Processors = *processors;
    cout << "Processor objects created at " << Processors << endl;

    Result<TLBContext *> tlbContexts
            = make_raw_array_counter<TLBContext>(numProcessors * tlbContextsPerProcessor);
    if (!tlbContexts) {
        return tlbContexts.status();
    }
    TLBContexts = *tlbContexts;

    cout << "TLBContext objects created at " << TLBContexts << endl;
    return Status::OK();
}

FutexFrameID CommonSystem::getNewFutexFrameID() {
    FutexFrameID result = __atomic_fetch_add(&nextFutexFrameID, 1, __ATOMIC_SEQ_CST);

    /* These IDs are combined with the low bits of the address to create the individual Futex IDs
     * inside the frame */
    assert(((nextFutexFrameID << PAGE_SHIFT) >> PAGE_SHIFT) == nextFutexFrameID);

    return result;
}

uint64_t CommonSystem::getNextTLBTimetamp() {
    return __atomic_fetch_add(&nextTLBTimestamp, 1, __ATOMIC_SEQ_CST);
}
