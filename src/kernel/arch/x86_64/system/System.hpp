#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <common/common.hpp>
#include <system/CommonSystem.hpp>

class System : public CommonSystem {
private:
    void setupSecondaryProcessorEntry(BootInfo *bootInfo);

public:
    PhysicalAddress ACPIRoot;
    PhysicalAddress secondaryProcessorEntry;

    System(BootInfo *bootInfo);
};

extern uint8_t SecondaryProcessorEntryCode;
extern uint8_t SecondaryProcessorEntryCodeEnd;
extern uint8_t SecondaryProcessorEntryMasterPageTable;

// make sure System fits in the fixed area assigned for it
static_assert(sizeof(System) < PAGE_SIZE - 6 * 8 - 22 * 8);

extern System CurrentSystem;

#endif // SYSTEM_HPP