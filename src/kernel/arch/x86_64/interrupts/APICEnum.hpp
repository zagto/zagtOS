#pragma once

#include <common/inttypes.hpp>

namespace apic {

enum class DeliveryMode : uint8_t {
    FIXED, LOWEST_PRIORITY, SMI, RESERVED1, NMI, INIT, STARTUP, RESERVED2
};
enum class Level : uint8_t {
    DEASSERT, ASSERT
};

}

