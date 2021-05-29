#include <system/Processor.hpp>
#include <paging/PagingContext.hpp>
#include <memory/DLMallocGlue.hpp>
#include <memory/TLBContext.hpp>
#include <system/System.hpp>

extern size_t KERNEL_STACK_SIZE;

Processor::KernelInterruptsLock Processor::kernelInterruptsLock;

void Processor::KernelInterruptsLock::lock() {
    basicDisableInterrupts();
    assert(CurrentProcessor->interruptsLockLocked == false);
    CurrentProcessor->interruptsLockLocked = true;
}

void Processor::KernelInterruptsLock::unlock() {
    assert(CurrentProcessor->interruptsLockLocked == true);
    CurrentProcessor->interruptsLockLocked = false;
    basicEnableInterrupts();
}

bool Processor::KernelInterruptsLock::isLocked() const {
    return CurrentProcessor->interruptsLockLocked;
}

Processor::Processor(size_t id, Status &status) :
        logBufferIndex{0},
        activeTLBContextID{CurrentSystem.tlbContextsPerProcessor * id},
        invalidateQueue{*this},
        id{id},
        scheduler(this, status),
        interrupts(id, status) {

    if (!status) {
        return;
    }

    TLBContexts[activeTLBContextID].activatePagingContext(&CurrentSystem.kernelOnlyPagingContext);

    Result<KernelVirtualAddress> address = DLMallocGlue.allocate(KERNEL_STACK_SIZE, 16);
    if (address) {
        kernelStack = address->asPointer<uint8_t>();
        status = Status::OK();
    } else {
        kernelStack = nullptr;
        status = address.status();
    }
}

Processor::~Processor() {
    // letting processors disappear is currently not supported
    Panic();
}

void Processor::sendInvalidateQueueProcessingIPI() {
    cout << "TODO: IPI sending" << endl;
    Panic();
}

TLBContextID Processor::activatePagingContext(PagingContext *pagingContext, TLBContextID tryFirst) {
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

    TLBContexts[tlbID].activate();
    return tlbID;
}
