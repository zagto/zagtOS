#pragma once

#include <common/Region.hpp>
#include <optional>
#include <setup/HandOverState.hpp>

namespace memoryMap {

void initialize();
uint8_t *allocateHandOver(size_t numPages);
/* on EFI platform, this method has to change the way output works, using these arguments */
void freezeAndExitFirmware(const hos_v1::SerialInfo &serialInfo,
                           const hos_v1::FramebufferInfo &framebufferInfo);

optional<Region> firstAvailableRegion();
optional<Region> nextAvailableRegion();

optional<Region> firstReclaimableRegion();
optional<Region> nextReclaimableRegion();

}
