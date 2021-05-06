#include <memory/TLBContext.hpp>
#include <memory/PageOutContext.hpp>
#include <system/Processor.hpp>
#include <system/System.hpp>
#include <processes/Frame.hpp>

extern "C" void basicSwitchToTLBContext(size_t localID);

TLBContext::TLBContext(TLBContextID id, Status &):
    processorID{static_cast<uint16_t>(id / CurrentSystem.tlbContextsPerProcessor)},
    localID{static_cast<uint16_t>(id % CurrentSystem.tlbContextsPerProcessor)} {}

Processor &TLBContext::processor() const {
    return Processors[processorID];
}

TLBContextID TLBContext::id() const {
    return static_cast<TLBContextID>(processorID) * CurrentSystem.tlbContextsPerProcessor
            + localID;
}

void TLBContext::activatePagingContext(PagingContext *pagingContext) {
    scoped_lock sl(processor().tlbContextsLock);
    activePagingContext = pagingContext;
    assert(CurrentSystem.tlbContextsPerProcessor == 1); // TODO
}

void TLBContext::remove(PagingContext *pagingContext) {
    scoped_lock sl(processor().tlbContextsLock);
    if (activePagingContext == pagingContext) {
        activePagingContext = nullptr;
    }
    assert(CurrentSystem.tlbContextsPerProcessor == 1); // TODO
}

bool TLBContext::potentiallyHolds(PagingContext *pagingContext) {
    scoped_lock sl(processor().tlbContextsLock);
    return activePagingContext == pagingContext;
}

PageOutContext TLBContext::requestInvalidate(Frame *frame, UserVirtualAddress address) {
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
