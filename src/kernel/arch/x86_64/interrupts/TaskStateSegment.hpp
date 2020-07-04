#ifndef TASKSTATESEGMENT_HPP
#define TASKSTATESEGMENT_HPP

#include <common/common.hpp>

class Thread;
template<typename T> class shared_ptr;

class __attribute__((__packed__)) TaskStateSegment
{
public:
    uint32_t ununsed0{0};
    uint32_t rspLow{0};
    uint32_t rspHigh{0};
    uint32_t unused1[22]{0};
    uint16_t unused2{0};
    uint16_t ioBase{0};

public:
    void update(Thread *thread);
};

#endif // TASKSTATESEGMENT_HPP
