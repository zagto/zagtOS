#pragma once

#include <system/CommonProcessor.hpp>
#include <interrupts/LocalAPIC.hpp>

class Processor : public CommonProcessor {
private:
    TaskStateSegment tss;
    apic::LocalAPIC localAPIC;

public:
    Processor();

    void localInitialization() noexcept;

    void sendIPI(IPI ipi) noexcept;
    void endOfInterrupt() noexcept;

    [[noreturn]] void returnToUserMode() noexcept;
    [[noreturn]] void returnInsideKernelMode(RegisterState *state) noexcept;
};

extern "C" Processor *CurrentProcessor();
void InitCurrentProcessorPointer(Processor *processor);
extern bool ProcessorsInitialized;
