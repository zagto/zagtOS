#pragma once

#include <common/common.hpp>
#include <system/CommonSystem.hpp>

#define SYSTEM_X86_64 1

class System : public CommonSystem {
private:
    void setupSecondaryProcessorEntry(const hos_v1::System &handOver);

public:
    PhysicalAddress ACPIRoot;
    PhysicalAddress secondaryProcessorEntry;

    System(hos_v1::System handOver);
};

extern uint8_t SecondaryProcessorEntryCode;
extern uint8_t SecondaryProcessorEntryCodeEnd;
extern uint8_t SecondaryProcessorEntryMasterPageTable;

extern System CurrentSystem;

