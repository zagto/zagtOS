#pragma once

#include <system/CommonProcessor.hpp>

class Processor : public CommonProcessor {
public:
    Processor();

    void localInitialization() noexcept;

    void sendIPI(IPI ipi) noexcept;
    void endOfInterrupt() noexcept;

    [[noreturn]] void returnToUserMode() noexcept;
    [[noreturn]] void returnInsideKernelMode(RegisterState *state) noexcept;
};

extern "C" Processor *CurrentProcessor();
extern "C" void InitCurrentProcessorPointer(Processor *processor);
extern bool ProcessorsInitialized;
