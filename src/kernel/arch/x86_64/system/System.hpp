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

    GlobalDescriptorTable gdt;
    InterruptDescriptorTable idt;
    LegacyPIC legacyPIC;
    vector<apic::IOAPIC> ioApics;

    void setupSyscalls() noexcept;
    void detectIoApics() noexcept;

public:
    PhysicalAddress ACPIRoot;

    System();

    void setupCurrentProcessor() noexcept;
};

extern System CurrentSystem;
extern hos_v1::System *_HandOverSystem;
