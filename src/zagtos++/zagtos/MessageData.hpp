#pragma once
/* Separate header for MessageData class used by ZBON with minimal dependencies.
 * Implementation of this class is in Messaging.cpp */

#include <cstdint>
#include <zagtos/KernelApi.h>

namespace zbon {
struct Size;
}

namespace zagtos {

struct MessageData : public cApi::ZoMessageData {
    constexpr MessageData(uint8_t *data,
                          size_t size,
                          size_t numHandles,
                          bool allocatedExternally = false) {
        this->data = data;
        this->size = size;
        this->numHandles = numHandles;
        this->allocatedExternally = allocatedExternally;
    }

    constexpr MessageData() {
        data = nullptr;
        size = 0;
        numHandles = 0;
        allocatedExternally = false;
    }
    MessageData(MessageData &other) = delete;
    MessageData(MessageData &&other);
    ~MessageData();
    zbon::Size ZBONSize() const;
};

}
