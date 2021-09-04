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
    void sendInvalidateQueueProcessingIPI();
    void endOfInterrupt();

    [[noreturn]] void returnToUserMode();
    [[noreturn]] void returnInsideKernelMode(RegisterState *state);
};
