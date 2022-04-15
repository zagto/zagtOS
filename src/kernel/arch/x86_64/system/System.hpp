#pragma once

#include <common/common.hpp>
#include <system/CommonSystem.hpp>
#include <interrupts/LegacyPic.hpp>
#include <interrupts/IOAPIC.hpp>
#include <interrupts/GlobalDescriptorTable.hpp>
#include <interrupts/InterruptDescriptorTable.hpp>

#define SYSTEM_X86_64 1

static constexpr size_t MAX_NUM_PROCESSORS = 256;

class PlatformInterrupt;

class System : public CommonSystem {
private:
    friend class Processor;
    friend __attribute__((noreturn)) void _handleInterrupt(RegisterState *registerState);

    GlobalDescriptorTable gdt;
    InterruptDescriptorTable idt;
    LegacyPIC legacyPIC;
    vector<apic::IOAPIC> ioApics;

    void setupSyscalls() noexcept;
    void detectIOAPICs();
    apic::IOAPIC &IOAPICForGSI(uint32_t gsi);

public:
    PhysicalAddress ACPIRoot;

    /* Called directly at the beginning, before Processors management is set up */
    System();
    /* Called once on each Processor once there it is on a good stack */
    void setupCurrentProcessor() noexcept;
    /* Called afterwards */
    void lateInitialization();

    void bindInterrupt(BoundInterrupt &boundInterrupt);
    void unbindInterrupt(BoundInterrupt &boundInterrupt);
    void interruptFullyProcessed(BoundInterrupt &boundInterrupt);
};

extern System CurrentSystem;
extern hos_v1::System *_HandOverSystem;
