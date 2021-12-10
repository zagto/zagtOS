#pragma once

#include <system/CommonProcessor.hpp>

enum IPI {
    CheckScheduler = 1u << 0,
    InvalidateQueueProcessing = 1u << 1
};

class Processor : public CommonProcessor {
private:
    TaskStateSegment tss;
    LocalAPIC localAPIC;

public:
    Processor(size_t id, Status &status);

    void localInitialization();

    void sendIPI(IPI ipi);
    void endOfInterrupt();

    [[noreturn]] void returnToUserMode();
    [[noreturn]] void returnInsideKernelMode(RegisterState *state);
};

extern "C" Processor *CurrentProcessor();
void InitCurrentProcessorPointer(Processor *processor);
extern bool ProcessorsInitialized;
