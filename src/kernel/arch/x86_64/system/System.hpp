#pragma once

#include <common/common.hpp>
#include <system/CommonSystem.hpp>
#include <interrupts/LegacyPic.hpp>
#include <interrupts/IOAPIC.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>

#define SYSTEM_X86_64 1

static constexpr size_t MAX_NUM_PROCESSORS = 256;

class System : public CommonSystem {
private:
    friend class Processor;
    friend __attribute__((noreturn)) void _handleInterrupt(RegisterState *registerState);

    struct LegacyIRQSetup {
        uint32_t gsi{0};
        apic::Polarity polarity;
        apic::TriggerMode triggerMode;
    };

    GlobalDescriptorTable gdt;
    InterruptDescriptorTable idt;
    LegacyPIC legacyPIC;
    vector<apic::IOAPIC> ioApics;
    LegacyIRQSetup legacyIRQs[0xff];

    void setupSyscalls() noexcept;
    void detectIOAPICs();
    void detectIRQSourceOverride() noexcept;

public:
    PhysicalAddress ACPIRoot;

    /* Called directly at the beginning, before Processors management is set up */
    System();
    /* Called once on each Processor once there it is on a good stack */
    void setupCurrentProcessor() noexcept;
    /* Called afterwards */
    void lateInitialization();
};

extern System CurrentSystem;
extern hos_v1::System *_HandOverSystem;
