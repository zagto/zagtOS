#pragma once

#include <common/inttypes.hpp>

namespace apic {

enum class DeliveryMode : uint8_t {
    FIXED, LOWEST_PRIORITY, SMI, RESERVED1, NMI, INIT, STARTUP, RESERVED2
};
enum class TriggerMode : uint8_t {
    EDGE, LEVEL
};
enum class Polarity : uint8_t {
    ACRIVE_HIGH, ACTIVE_LOW
};
enum class Level : uint8_t {
    DEASSERT, ASSERT
};

}

