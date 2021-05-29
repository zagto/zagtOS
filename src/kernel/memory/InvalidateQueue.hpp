#pragma once

#include <common/common.hpp>
#include <queue>

class Frame;
class Status;

class InvalidateQueue {
private:
    struct Item {
        TLBContextID tlbContextID;
        Frame *frame;
        VirtualAddress address;
        uint64_t timestamp;
    };

    Processor &processor;
    SpinLock lock;
    queue<Item> items;
    uint64_t processedUntilTimestamp{0};

    void _localProcessing(optional<Item> extraItem);

public:
    InvalidateQueue(Processor &processor);

    uint64_t add(TLBContextID tlbContextID,
                 Frame *frame,
                 UserVirtualAddress address);
    void ensureProcessedUntil(uint64_t timestamp, optional<Item> extraItem = {});
    void localProcessing();
};
