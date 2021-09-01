#include <portio.hpp>
#include <LegacyTimer.hpp>

using namespace portio;

void LegacyTimer::setReloadValue(uint16_t value) {
    OutB(DATA_PORT, value & 0xff);
    OutB(DATA_PORT, value >> 8);
}


uint16_t LegacyTimer::readValue() {
    uint16_t low = InB(DATA_PORT);
    uint16_t high = InB(DATA_PORT);
    return (high << 8) | low;
}


void LegacyTimer::setOperatingMode() {
    /* channel = 0
     * access mode = low-byte/high-byte
     * operating mode = 0 */
    OutB(COMMAND_PORT, 0b11u << 4);
}
