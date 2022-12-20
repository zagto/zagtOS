#pragma once
#include <common/inttypes.hpp>
#include <common/addresses.hpp>
#include <setup/HandOverState.hpp>

void MapLoaderMemory();
void MapFramebufferMemory(hos_v1::FramebufferInfo &framebufferInfo);
void MapSerialMemory(hos_v1::SerialInfo &framebufferInfo);
void CreateIdentityMap(PhysicalAddress maxPhysicalAddress);
