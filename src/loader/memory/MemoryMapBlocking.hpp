#pragma once
#include <common/inttypes.hpp>
#include <common/Region.hpp>
#include <optional>

namespace memoryMapBlocking {
using GetRegionCallback = optional<Region> (*)(size_t index);

optional<Region> findNextAvailableRegion(bool reset,
                                         size_t minimumSize);
void initialize(size_t numFirmwareRegions, GetRegionCallback callback);
void blockRegion(Region region, bool reclaimable);
uint8_t *allocateHandOver(size_t numPages);
optional<Region> nextReclaimableRegion(bool reset);
}
