#pragma once

#include <common/common.hpp>
#include <queue>

class Frame;
class Status;
class CommonProcessor;

class InvalidateQueue {
private:
    struct Item {
        TLBContextID tlbContextID;
        Frame *frame;
        VirtualAddress address;
        uint64_t timestamp;
    };

    CommonProcessor &processor;
    SpinLock lock;
    queue<Item> items;
    uint64_t processedUntilTimestamp{0};

    void _localProcessing(optional<Item> extraItem);

public:
    InvalidateQueue(CommonProcessor &processor);

    uint64_t add(TLBContextID tlbContextID,
                 Frame *frame,
                 UserVirtualAddress address);
    void ensureProcessedUntil(uint64_t timestamp, optional<Item> extraItem = {});
    void localProcessing();
};
