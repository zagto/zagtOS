#include <memory/InvalidateQueue.hpp>
#include <system/System.hpp>
#include <memory/TLBContext.hpp>
#include <system/Processor.hpp>
#include <processes/Frame.hpp>

InvalidateQueue::InvalidateQueue(CommonProcessor &processor) noexcept:
    processor{processor} {}

void InvalidateQueue::_localProcessing() {
    Item item;
    if (extraItem) {
        item = *extraItem;
        extraItem = {};
    } else {
        if (items.empty()) {
            uint64_t newTimestamp = CurrentSystem.getNextTLBTimetamp();
            /* Use atomic since ensureProcessedUntil reads this without holding lock */
            __atomic_store(&processedUntilTimestamp, &newTimestamp, __ATOMIC_SEQ_CST);
            return;
        }
        item = items.top();
        items.pop();
    }

    while (true) {
        assert(item.tlbContextID != TLB_CONTEXT_ID_NONE);

        TLBContexts[item.tlbContextID].localInvalidate(
                    UserVirtualAddress(item.address.value()));
        assert(item.frame != nullptr);
        item.frame->decreaseInvalidateRequestReference();

        if (items.empty()) {
            /* Create a new timestamp, since the most recent we processed may still be older than
             * what exists on other CPUs */
            uint64_t newTimestamp = CurrentSystem.getNextTLBTimetamp();
            /* Use atomic since ensureProcessedUntil reads this without holding lock */
            __atomic_store(&processedUntilTimestamp, &newTimestamp, __ATOMIC_SEQ_CST);
            return;
        }
        item = items.top();
        items.pop();
    }
}

uint64_t InvalidateQueue::add(TLBContextID tlbContextID,
                              Frame *frame,
                              UserVirtualAddress address) noexcept {

    assert(tlbContextID != TLB_CONTEXT_ID_NONE);

    while (true) {
        bool gotExtraItem = false;
        size_t processUntilTimestamp = 0;
        {
            scoped_lock sl(lock);

            uint64_t timestamp = CurrentSystem.getNextTLBTimetamp();
            Item newItem{tlbContextID, frame, address, timestamp};

            try {
                items.push_back(newItem);
                return timestamp;
            } catch (...) {
                /* If it's impossible to add an item to the queue because of the memory situation, we have
                 * to force processing right now. This item which can't be added to the queue is what the
                 * extraItem parameter in the processing functions is for. */
            }

            /* There is an extraItem variable for OOM cases. But only one add()-call can use it
             * at a time, so use it if it is free, otherwise wait until the other extraItem is
             * processed and try again. */
            if (!extraItem) {
                extraItem = newItem;
                gotExtraItem = true;
            }
            processUntilTimestamp = extraItem->timestamp;
        } /* enf of lock scope */

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

void InvalidateQueue::ensureProcessedUntil(uint64_t timestamp) noexcept {
    uint64_t procesedTimestampLocal;
    __atomic_load(&processedUntilTimestamp, &procesedTimestampLocal, __ATOMIC_SEQ_CST);
    if (procesedTimestampLocal >= timestamp) {
        return;
    }

    if (CurrentProcessor()->id == processor.id) {
        scoped_lock sl(lock);
        _localProcessing();
    } else {
        while (procesedTimestampLocal < timestamp) {
            static_cast<Processor &>(processor).sendIPI(IPI::InvalidateQueueProcessing);

            for (size_t counter = 0; counter < 100000 && procesedTimestampLocal < timestamp; counter++) {
                /* busy waiting */
                __atomic_load(&processedUntilTimestamp, &procesedTimestampLocal, __ATOMIC_SEQ_CST);
            }
        }
    }
}

void InvalidateQueue::localProcessing() noexcept {
    assert(CurrentProcessor()->id == processor.id);

    scoped_lock sl(lock);
    _localProcessing();
}

