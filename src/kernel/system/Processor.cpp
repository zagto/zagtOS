#include <system/Processor.hpp>
#include <paging/PagingContext.hpp>
#include <memory/DLMallocGlue.hpp>
#include <memory/TLBContext.hpp>
#include <system/System.hpp>

extern size_t KERNEL_STACK_SIZE;

Processor::Processor(size_t id, Status &status) :
        logBufferIndex{0},
        nextLocalTlbContextID{0},
        invalidateQueue{*this},
        id{id},
        scheduler(this, status),
        interrupts(id, status),
        activeTLBContextID{CurrentSystem.tlbContextsPerProcessor * id} {

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
