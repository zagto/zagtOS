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
    optional<Item> extraItem;
    uint64_t processedUntilTimestamp{0};

    void _localProcessing();

public:
    InvalidateQueue(CommonProcessor &processor) noexcept;

    uint64_t add(TLBContextID tlbContextID,
                 Frame *frame,
                 UserVirtualAddress address) noexcept;
    void ensureProcessedUntil(uint64_t timestamp) noexcept;
    void localProcessing() noexcept;
};
