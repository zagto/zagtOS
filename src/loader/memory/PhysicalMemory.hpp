#pragma once
#include <memory/FrameStack.hpp>
#include <common/addresses.hpp>

extern FrameStack DirtyFrameStack;
extern FrameStack CleanFrameStack;

extern PhysicalAddress SecondaryProcessorEntry;

PhysicalAddress InitPhysicalFrameManagement();
PhysicalAddress AllocatePhysicalFrame();
void FreePhysicalFrame(PhysicalAddress frame);
void ClearFrame(void *frame);

