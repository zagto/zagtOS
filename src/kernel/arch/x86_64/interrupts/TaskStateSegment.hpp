#ifndef TASKSTATESEGMENT_HPP
#define TASKSTATESEGMENT_HPP

#include <common/common.hpp>

class Thread;

class __attribute__((__packed__)) TaskStateSegment
{
public:
    u32 ununsed0{0};
    u32 rspLow{0};
    u32 rspHigh{0};
    u32 unused1[22]{0};
    u16 unused2{0};
    u16 ioBase{0};

public:
    void update(Thread *thread);
};

#endif // TASKSTATESEGMENT_HPP
