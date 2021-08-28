#pragma once
#include <common/common.hpp>

class LegacyTimer {
    static const uint16_t DATA_PORT = 0x40;
    static const uint16_t COMMAND_PORT = 0x43;

public:
    void setReloadValue(uint16_t value);
    void setOperatingMode();
    uint16_t readValue();
};
