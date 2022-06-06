#pragma once

#include <common/inttypes.hpp>

struct TriggerMode {
    /* These are defined to be the same values as for the x86 APIC. If you change the enum, you have
     * to add conversion code there */
    enum {
        LEVEL_LOW, LEVEL_HIGH, FALLING_EDGE, RISING_EDGE
    };

    size_t id;

    TriggerMode(size_t id) : id{id} {}
    uint32_t x86Polarity() const { return static_cast<uint32_t>(id == LEVEL_LOW || id == FALLING_EDGE); }
    uint32_t x86TriggerMode() const { return static_cast<uint32_t>(id == LEVEL_LOW || id == LEVEL_HIGH); }
    bool isLevel() const {
        return id == TriggerMode::LEVEL_LOW || id == TriggerMode::LEVEL_HIGH;
    }
};
