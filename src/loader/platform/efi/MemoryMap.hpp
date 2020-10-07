#pragma once

#include <common/Region.hpp>
#include <optional>

namespace memoryMap {

uint8_t *allocateHandOver(size_t numPages);

void freezeAndExitFirmware();

optional<Region> firstAvailableRegion();
optional<Region> nextAvailableRegion();

optional<Region> firstReclaimableRegion();
optional<Region> nextReclaimableRegion();

}
