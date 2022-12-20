#pragma once

#include <common/common.hpp>
#include <Paging.hpp>

extern uint8_t SecondaryProcessorEntryCode;
extern uint8_t SecondaryProcessorEntryCodeEnd;
extern uint8_t SecondaryProcessorEntryMasterPageTable;
extern uint8_t SecondaryProcessorEntryStackPointerPointer;
extern uint8_t SecondaryProcessorEntryTarget;

extern size_t BootProcessorHardwareID;

namespace hos_v1 {
struct PagingContext;
}

size_t secondaryProcessorEntryCodeLength();
size_t startSecondaryProcessors();
void releaseSecondaryProcessorsToKernel();
