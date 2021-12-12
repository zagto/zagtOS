#pragma once

#include <common/inttypes.hpp>

namespace apic {

enum class DeliveryMode : uint32_t {
    FIXED, LOWEST_PRIORITY, SMI, RESERVED1, NMI, INIT, STARTUP, RESERVED2
};
enum class TriggerMode : uint32_t {
    EDGE, LEVEL
};
enum class Polarity : uint32_t {
    ACRIVE_HIGH, ACTIVE_LOW
};
enum class Level : uint32_t {
    DEASSERT, ASSERT
};

}

