#pragma once

#include <system/CommonProcessor.hpp>

class Processor : public CommonProcessor {
private:
    TaskStateSegment tss;
    LocalAPIC localAPIC;

public:
    Processor(size_t id, Status &status);

    void localInitialization();

    void sendCheckSchedulerIPI();

    __attribute__((noreturn)) void returnToUserMode();
};
