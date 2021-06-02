#pragma once

#include <common/common.hpp>
#include <system/CommonSystem.hpp>
#include <interrupts/LegacyPIC.hpp>
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

    void setupSyscalls();

public:
    PhysicalAddress ACPIRoot;

    System();

    void setupCurrentProcessor();
};

extern System CurrentSystem;
extern hos_v1::System *_HandOverSystem;
