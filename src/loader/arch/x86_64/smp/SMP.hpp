#pragma once

#include <common/common.hpp>
#include <Paging.hpp>

extern uint8_t SecondaryProcessorEntryCode;
extern uint8_t SecondaryProcessorEntryCodeEnd;
extern uint8_t SecondaryProcessorEntryMasterPageTable;

extern size_t BootProcessorHardwareID;

size_t secondaryProcessorEntryCodeLength();
size_t startSecondaryProcessors();
void releaseSecondaryProcessorsToKernel();
