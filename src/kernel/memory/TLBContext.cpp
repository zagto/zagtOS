#include <memory/TLBContext.hpp>
#include <memory/PageOutContext.hpp>
#include <system/Processor.hpp>
#include <system/System.hpp>
#include <processes/Frame.hpp>

extern "C" void basicSwitchToTLBContext(size_t localID);

TLBContext::TLBContext(TLBContextID id, Status &):
        processorID{static_cast<uint16_t>(id / CurrentSystem.tlbContextsPerProcessor)},
        localID{static_cast<uint16_t>(id % CurrentSystem.tlbContextsPerProcessor)} {

    size_t tcpp = CurrentSystem.tlbContextsPerProcessor;

    nextLocalID = (localID + 1) % tcpp;
    previousLocalID = (localID + tcpp - 1) % tcpp;

    activePagingContext = &CurrentSystem.kernelOnlyPagingContext;
}

Processor &TLBContext::processor() const {
    return Processors[processorID];
}

TLBContextID TLBContext::localIDToGlobal(uint16_t localID, uint16_t processorID) {
    return static_cast<TLBContextID>(processorID) * CurrentSystem.tlbContextsPerProcessor
            + localID;
}

TLBContextID TLBContext::id() const {
    return localIDToGlobal(localID, processorID);
}

void TLBContext::activate() {
    /* TODO: current PCID/ASID setting goes here */

    TLBContextID oldID = CurrentProcessor->activeTLBContextID;
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

    CurrentProcessor->activeTLBContextID = id();
}

void TLBContext::activatePagingContext(PagingContext *pagingContext) {
    assert(processor().tlbContextsLock.isLocked());

    if (CurrentProcessor->activeTLBContextID != id()) {
        activate();
    }
    if (pagingContext != activePagingContext) {
        /* sets the address in CR3 on x86_64 */
        pagingContext->activate();
    }
    activePagingContext = pagingContext;


}

void TLBContext::remove(PagingContext *pagingContext) {
    scoped_lock sl(processor().tlbContextsLock);
    if (activePagingContext == pagingContext) {
        activePagingContext = nullptr;

        /* position ourselves at the "start" of the cycle as this context is free now */
        TLBContextID activeID = CurrentProcessor->activeTLBContextID;
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

bool TLBContext::potentiallyHolds(PagingContext *pagingContext) {
    scoped_lock sl(processor().tlbContextsLock);
    return activePagingContext == pagingContext;
}

extern "C" void basicInvalidateTLBContext(size_t localTLBContextID, UserVirtualAddress address);

void TLBContext::localInvalidate(UserVirtualAddress address) {
    basicInvalidateTLBContext(localID, address);
}

PageOutContext TLBContext::requestInvalidate(Frame *frame, UserVirtualAddress address) {
    assert(Processor::kernelInterruptsLock.isLocked());
    assert(CurrentProcessor->interruptsLockLocked);

    if (processorID == CurrentProcessor->id) {
        localInvalidate(address);
        frame->decreaseInvalidateRequestReference();
        return {};
    } else {
        cout << "using InvalidateList for address " << address.value()
             << " in TLB " << id() << " which is on Processor " << processorID
             << " while running on Processor " << CurrentProcessor->id << endl;
        uint64_t timestamp = processor().invalidateQueue.add(id(), frame, address);
        return {{processor()}, timestamp};
    }
}
