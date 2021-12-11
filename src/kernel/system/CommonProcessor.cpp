#include <system/Processor.hpp>
#include <paging/PagingContext.hpp>
#include <memory/TLBContext.hpp>
#include <system/System.hpp>
#include <memory>
#include <interrupts/KernelInterruptsLock.hpp>


static size_t nextProcessorID = 0;

CommonProcessor::CommonProcessor() :
        self{this},
        logBufferIndex{0},
        activeTLBContextID{CurrentSystem.tlbContextsPerProcessor * id},
        invalidateQueue{*this},
        id{nextProcessorID++},
        scheduler(this) {

    cout << "initializing Processor " << id << " at " << this << endl;
    kernelStack = make_shared<KernelStack>(RegisterState());
}

CommonProcessor::~CommonProcessor() {
    // letting processors disappear is currently not supported
    Panic();
}

TLBContextID CommonProcessor::activatePagingContext(PagingContext *pagingContext,
                                                    TLBContextID tryFirst) noexcept {
    assert(KernelInterruptsLock.isLocked());
    scoped_lock sl(tlbContextsLock);

    TLBContextID tlbID;

    if (tryFirst != TLB_CONTEXT_ID_NONE
            && TLBContexts[tryFirst].activePagingContext == pagingContext) {
        tlbID = tryFirst;
    } else {
        tlbID = TLBContext::localIDToGlobal(TLBContexts[activeTLBContextID].nextLocalID, id);
    }

    assert(TLBContexts[tlbID].processor().id == CurrentProcessor()->id);

    TLBContexts[tlbID].activatePagingContext(pagingContext);
    return tlbID;
}

Thread *CommonProcessor::activeThread() const noexcept {
    assert(scheduler.lock.isLocked()
           || (KernelInterruptsLock.isLocked() && CurrentProcessor() == this));
    return _activeThread;
}

void CommonProcessor::activeThread(Thread *newActiveThread) noexcept {
    assert(scheduler.lock.isLocked() && CurrentProcessor() == this);
    _activeThread = newActiveThread;
    if (newActiveThread != nullptr) {
        userRegisterState = newActiveThread->kernelStack->userRegisterState();
    } else {
        userRegisterState = nullptr;
    }
}
