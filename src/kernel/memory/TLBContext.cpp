#include <memory/TLBContext.hpp>
#include <memory/PageOutContext.hpp>
#include <system/Processor.hpp>
#include <system/System.hpp>
#include <processes/Frame.hpp>
#include <interrupts/KernelInterruptsLock.hpp>

extern "C" void basicSwitchToTLBContext(size_t localID);

static size_t nextInstanceID = 0;

TLBContext::TLBContext():
        processorID{static_cast<uint16_t>(nextInstanceID / CurrentSystem.tlbContextsPerProcessor)},
        localID{static_cast<uint16_t>(nextInstanceID % CurrentSystem.tlbContextsPerProcessor)} {

    nextInstanceID++;

    size_t tcpp = CurrentSystem.tlbContextsPerProcessor;

    nextLocalID = (localID + 1) % tcpp;
    previousLocalID = (localID + tcpp - 1) % tcpp;

    activePagingContext = &CurrentSystem.kernelOnlyPagingContext;
}

Processor &TLBContext::processor() const noexcept {
    return Processors[processorID];
}

TLBContextID TLBContext::localIDToGlobal(uint16_t localID, uint16_t processorID) noexcept {
    return static_cast<TLBContextID>(processorID) * CurrentSystem.tlbContextsPerProcessor
            + localID;
}

TLBContextID TLBContext::id() const noexcept {
    return localIDToGlobal(localID, processorID);
}

void TLBContext::activate() noexcept {
    /* TODO: current PCID/ASID setting goes here */

    TLBContextID oldID = processor().activeTLBContextID;
    assert(oldID != id());

    if (TLBContexts[oldID].nextLocalID != localID) {
        /* We're not next in cycle, Position ourselves at the "start" of the cycle */

        /* unlink from current position */
        TLBContexts[localIDToGlobal(previousLocalID, processorID)].nextLocalID = nextLocalID;
        TLBContexts[localIDToGlobal(nextLocalID, processorID)].previousLocalID = previousLocalID;

        /* insert */
        nextLocalID = TLBContexts[oldID].nextLocalID;
        previousLocalID = oldID % CurrentSystem.tlbContextsPerProcessor;

        TLBContexts[localIDToGlobal(nextLocalID, processorID)].previousLocalID = localID;
        TLBContexts[oldID].nextLocalID = localID;
    }

    processor().activeTLBContextID = id();
}

void TLBContext::activatePagingContext(PagingContext *pagingContext) noexcept {
    assert(processor().tlbContextsLock.isLocked());

    if (processor().activeTLBContextID != id()) {
        activate();
    }
    if (pagingContext != activePagingContext) {
        /* sets the address in CR3 on x86_64 */
        pagingContext->activate();
    }
    activePagingContext = pagingContext;


}

void TLBContext::remove(PagingContext *pagingContext) noexcept {
    scoped_lock sl(processor().tlbContextsLock);
    if (activePagingContext == pagingContext) {
        activePagingContext = nullptr;

        /* position ourselves at the "start" of the cycle as this context is free now */
        TLBContextID activeID = processor().activeTLBContextID;
        if (TLBContexts[activeID].nextLocalID != localID) {
            /* We're not next in cycle, Position ourselves at the "start" of the cycle */

            /* unlink from current position */
            TLBContexts[localIDToGlobal(previousLocalID, processorID)].nextLocalID = nextLocalID;
            TLBContexts[localIDToGlobal(nextLocalID, processorID)].previousLocalID = previousLocalID;

            /* insert */
            nextLocalID = TLBContexts[activeID].nextLocalID;
            previousLocalID = activeID % CurrentSystem.tlbContextsPerProcessor;

            TLBContexts[localIDToGlobal(nextLocalID, processorID)].previousLocalID = localID;
            TLBContexts[activeID].nextLocalID = localID;
        }
    }
}

bool TLBContext::potentiallyHolds(PagingContext *pagingContext) noexcept {
    scoped_lock sl(processor().tlbContextsLock);
    return activePagingContext == pagingContext;
}

extern "C" void basicInvalidateTLBContext(size_t localTLBContextID, UserVirtualAddress address);

void TLBContext::localInvalidate(UserVirtualAddress address) noexcept {
    basicInvalidateTLBContext(localID, address);
}

PageOutContext TLBContext::requestInvalidate(Frame *frame,
                                             ProcessAddressSpace *addressSpace,
                                             UserVirtualAddress address) noexcept {
    assert(KernelInterruptsLock.isLocked());

    if (processorID == CurrentProcessor()->id) {
        localInvalidate(address);
        frame->decreaseInvalidateRequestReference();
        return PageOutContext(addressSpace); /* empty PageOutContext */
    } else {
        uint64_t timestamp = processor().invalidateQueue.add(id(), frame, address);
        return PageOutContext(addressSpace, {processor()}, timestamp);
    }
}
