#include <memory/InvalidateQueue.hpp>
#include <system/System.hpp>
#include <memory/TLBContext.hpp>
#include <system/Processor.hpp>
#include <processes/Frame.hpp>

InvalidateQueue::InvalidateQueue(CommonProcessor &processor):
    processor{processor} {}

void InvalidateQueue::_localProcessing() {
    Item item;
    if (extraItem) {
        item = *extraItem;
        extraItem = {};
    } else {
        if (items.empty()) {
            return;
        }
        item = items.top();
        items.pop();
    }

    size_t maxTimestamp = 0;

    while (true) {
        assert(item.tlbContextID != TLB_CONTEXT_ID_NONE);

        TLBContexts[item.tlbContextID].localInvalidate(
                    UserVirtualAddress(item.address.value()));
        assert(item.frame != nullptr);
        item.frame->decreaseInvalidateRequestReference();

        maxTimestamp = max(maxTimestamp, item.timestamp);

        if (items.empty()) {
            /* Use atomic since ensureProcessedUntil reads this without holding lock */
            __atomic_store(&processedUntilTimestamp, &maxTimestamp, __ATOMIC_SEQ_CST);
            return;
        }
        item = items.top();
        items.pop();
    }
}

uint64_t InvalidateQueue::add(TLBContextID tlbContextID,
                              Frame *frame,
                              UserVirtualAddress address) {

    assert(tlbContextID != TLB_CONTEXT_ID_NONE);

    while (true) {
        Status status;
        bool gotExtraItem = false;
        size_t processUntilTimestamp = 0;
        {
            scoped_lock sl(lock);

            uint64_t timestamp = CurrentSystem.getNextTLBTimetamp();
            Item newItem{tlbContextID, frame, address, timestamp};

            status = items.push_back(newItem);

            if (status) {
                return timestamp;
            }

            /* If it's impossible to add an item to the queue because of the memory situation, we have
             * to force processing right now. This item which can't be added to the queue is what the
             * extraItem parameter in the processing functions is for. */
            assert(status == Status::OutOfKernelHeap() || status == Status::OutOfMemory());

            /* There is an extraItem variable for OOM cases. But only one add()-call can use it
             * at a time, so use it if it is free, otherwise wait until the other extraItem is
             * processed and try again. */
            if (!extraItem) {
                extraItem = newItem;
                gotExtraItem = true;
            }
            processUntilTimestamp = extraItem->timestamp;
        } /* enf of lock scope */

        if (!status) {
            ensureProcessedUntil(processUntilTimestamp);

            /* If we got to set the extraItem, our extraItem got processed now and we can return.
             * otherwise at least processing cleared the extraItem variable. */
            if (gotExtraItem) {
                cout << "Forced processing of InvalidateQueue because of low memory situation." << endl;
                return processUntilTimestamp;
            } else {
                cout << "Forced processing of InvalidateQueue to clear extraItem variable." << endl;
            }
        }
    }
}

void InvalidateQueue::ensureProcessedUntil(uint64_t timestamp) {
    uint64_t procesedTimestampLocal;
    __atomic_load(&processedUntilTimestamp, &procesedTimestampLocal, __ATOMIC_SEQ_CST);
    if (procesedTimestampLocal >= timestamp) {
        return;
    }

    if (CurrentProcessor->id == processor.id) {
        scoped_lock sl(lock);
        _localProcessing();
    } else {
        static_cast<Processor &>(processor).sendInvalidateQueueProcessingIPI();

        while (procesedTimestampLocal < timestamp) {
            /* busy waiting */
            __atomic_load(&processedUntilTimestamp, &procesedTimestampLocal, __ATOMIC_SEQ_CST);
        }
    }
}

void InvalidateQueue::localProcessing() {
    assert(CurrentProcessor->id == processor.id);

    scoped_lock sl(lock);
    _localProcessing();
}

