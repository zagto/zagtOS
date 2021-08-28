#include <common/common.hpp>
#include <system/System.hpp>
#include <system/Processor.hpp>
#include <memory/TLBContext.hpp>

CommonSystem::CommonSystem(const hos_v1::System &handOver):
    nextFutexFrameID{handOver.nextFutexFrameID},
    time{handOver.timerFrequency},
    kernelOnlyPagingContext(handOver.handOverPagingContext, handOverStatus),
    futexManager(handOverStatus),
    numProcessors{handOver.numProcessors} {}

Status CommonSystem::initProcessorsAndTLB() {
    cout << "Initializing TLBContext and Processor structures..." << endl;

    Result<TLBContext *> tlbContexts
            = make_raw_array_counter<TLBContext>(numProcessors * tlbContextsPerProcessor);
    if (!tlbContexts) {
        return tlbContexts.status();
    }
    TLBContexts = *tlbContexts;

    cout << "TLBContext objects created at " << TLBContexts << endl;

    Result<Processor *> processors = make_raw_array_counter<Processor>(numProcessors);
    if (!processors) {
        return processors.status();
    }
    Processors = *processors;
    cout << "Processor objects created at " << Processors << endl;

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
