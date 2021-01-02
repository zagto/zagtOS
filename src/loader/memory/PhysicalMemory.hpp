#pragma once
#include <memory/FrameStack.hpp>
#include <common/addresses.hpp>

extern FrameStack DirtyFrameStack[hos_v1::DMAZone::COUNT];
extern FrameStack CleanFrameStack[hos_v1::DMAZone::COUNT];

extern PhysicalAddress SecondaryProcessorEntry;

PhysicalAddress InitPhysicalFrameManagement();
PhysicalAddress AllocatePhysicalFrame();
void FreePhysicalFrame(PhysicalAddress frame);
void ClearFrame(void *frame);

