#pragma once

#include <system/CommonProcessor.hpp>

class Processor : public CommonProcessor {
private:
    TaskStateSegment tss;
    LocalAPIC localAPIC;

public:
    Processor(size_t id, Status &status);

    __attribute__((noreturn)) void returnToUserMode();
};
