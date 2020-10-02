#pragma once

#include <common/Region.hpp>
#include <optional>

namespace memoryMap {

void freezeAndExitFirmware();

optional<Region> firstAvailableRegion();
optional<Region> nextAvailableRegion();

optional<Region> firstReclaimableRegion();
optional<Region> nextReclaimableRegion();

}
