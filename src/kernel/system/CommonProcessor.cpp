#include <system/Processor.hpp>
#include <paging/PagingContext.hpp>
#include <memory/TLBContext.hpp>
#include <system/System.hpp>
#include <memory>

CommonProcessor::KernelInterruptsLock CommonProcessor::kernelInterruptsLock;

void CommonProcessor::KernelInterruptsLock::lock() {
    basicDisableInterrupts();
    assert(CurrentProcessor->interruptsLockLocked == false);
    CurrentProcessor->interruptsLockLocked = true;
}

void CommonProcessor::KernelInterruptsLock::unlock() {
    assert(CurrentProcessor->interruptsLockLocked == true);
    CurrentProcessor->interruptsLockLocked = false;
    basicEnableInterrupts();
}

bool CommonProcessor::KernelInterruptsLock::isLocked() const {
    return CurrentProcessor->interruptsLockLocked;
}

CommonProcessor::CommonProcessor(size_t id, Status &status) :
        logBufferIndex{0},
        activeTLBContextID{CurrentSystem.tlbContextsPerProcessor * id},
        invalidateQueue{*this},
        id{id},
        scheduler(this, status) {
    if (!status) {
        return;
    }

    cout << "initializing Processor " << id << " at " << this << endl;

    Result kernelStack = make_shared<KernelStack>(RegisterState());
    if (kernelStack) {
        this->kernelStack = *kernelStack;
    } else {
        status = kernelStack.status();
    }
}

CommonProcessor::~CommonProcessor() {
    // letting processors disappear is currently not supported
    Panic();
}

void CommonProcessor::sendInvalidateQueueProcessingIPI() {
    cout << "TODO: IPI sending" << endl;
    Panic();
}

TLBContextID CommonProcessor::activatePagingContext(PagingContext *pagingContext,
                                                    TLBContextID tryFirst) {
    assert(interruptsLockLocked);
    scoped_lock sl(tlbContextsLock);

    TLBContextID tlbID;

    if (tryFirst != TLB_CONTEXT_ID_NONE
            && TLBContexts[tryFirst].activePagingContext == pagingContext) {
        tlbID = tryFirst;
    } else {
        tlbID = TLBContext::localIDToGlobal(TLBContexts[activeTLBContextID].nextLocalID, id);
    }

    assert(&TLBContexts[tlbID].processor() == CurrentProcessor);

    TLBContexts[tlbID].activatePagingContext(pagingContext);
    return tlbID;
}
