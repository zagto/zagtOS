#pragma once

#include <common/common.hpp>

class Thread;

class __attribute__((__packed__)) TaskStateSegment
{
private:
    static const size_t IST_STACK_QWORDS{6};
    uint32_t ununsed0{0};
    uint64_t rsp;
    uint32_t unused1[6]{0};
    uint64_t *IST1{nullptr};
    uint64_t *IST2{nullptr};
    uint32_t unused2[12]{0};
    uint16_t unused3{0};
    uint16_t ioBase{0};

public:
    TaskStateSegment(Status &status);
    ~TaskStateSegment();

    void update(Thread *thread);
};

