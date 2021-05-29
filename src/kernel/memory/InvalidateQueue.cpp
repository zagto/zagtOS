#include <memory/InvalidateQueue.hpp>
#include <system/System.hpp>
#include <memory/TLBContext.hpp>

InvalidateQueue::InvalidateQueue(Processor &processor):
    processor{processor} {}

extern "C" void basicInvalidate(KernelVirtualAddress address);
extern "C" void basicInvalidateTLBContext(size_t localTLBContextID, UserVirtualAddress address);

void InvalidateQueue::_localProcessing(optional<Item> extraItem) {
    Item item;
    if (extraItem) {
        item = *extraItem;
    } else {
        if (items.empty()) {
            return;
        }
        item = items.top();
        items.pop();
    }

    do {
        if (item.tlbContextID == TLB_CONTEXT_ID_NONE) {
            basicInvalidate(KernelVirtualAddress(item.address.value()));
        } else {
            basicInvalidateTLBContext(item.tlbContextID % CurrentSystem.tlbContextsPerProcessor,
                                      UserVirtualAddress(item.address.value()));
        }

        item = items.top();
        items.pop();
    } while (!items.empty());
}

uint64_t InvalidateQueue::add(TLBContextID tlbContextID,
                              Frame *frame,
                              UserVirtualAddress address) {
    scoped_lock sl(lock);

    uint64_t timestamp = CurrentSystem.getNextTLBTimetamp();
    Item newItem{tlbContextID, frame, address, timestamp};
    Status status = items.push_back(newItem);
    if (!status) {
        /* If it's impossible to add an item to the queue because of the memory situation, we have
         * to force processing right now. This item which can't be added to the queue is what the
         * extraItem parameter in the processing functions is for. */
        assert(status == Status::OutOfKernelHeap() || status == Status::OutOfMemory());

        cout << "Force processing of InvalidateQueue because of low memory situation." << endl;
        ensureProcessedUntil(timestamp, newItem);
    }
    return timestamp;
}

void InvalidateQueue::ensureProcessedUntil(uint64_t timestamp, optional<Item> extraItem) {
    scoped_lock sl(lock);

    if (processedUntilTimestamp >= timestamp) {
        return;
    }

    if (CurrentProcessor->id == processor.id) {
        _localProcessing(extraItem);
    }
}

void InvalidateQueue::localProcessing() {
    assert(Processor::kernelInterruptsLock.isLocked());
    assert(CurrentProcessor->id == processor.id);

    scoped_lock sl(lock);
    _localProcessing({});
}
