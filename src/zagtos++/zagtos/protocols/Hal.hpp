#pragma once

#include <zagtos/UUID.hpp>
#include <zagtos/Messaging.hpp>

namespace zagtos {
namespace hal {

static constexpr UUID MSG_START(
    0x4b, 0x87, 0xe1, 0xfc, 0x7f, 0x7a, 0x4f, 0x1c,
    0x9f, 0x1a, 0xaf, 0x3a, 0x38, 0x6d, 0x79, 0x89);
static constexpr UUID MSG_START_RESULT(
    0xa8, 0x24, 0x82, 0xcf, 0x8c, 0xde, 0x43, 0x03,
    0xb4, 0x45, 0x3b, 0xf0, 0x56, 0x4a, 0xb6, 0xea);

static constexpr UUID CONTROLLER_TYPE_PCI(
            0xdb, 0x5a, 0x9e, 0x01, 0xa5, 0x4e, 0x46, 0xd4,
            0xb7, 0x91, 0x0c, 0x10, 0xd5, 0xc7, 0xbe, 0x6f);
static constexpr UUID CONTROLLER_TYPE_PS2(
            0x8f, 0xbb, 0x96, 0x55, 0x47, 0xbb, 0x4f, 0x1f,
            0xbf, 0x38, 0x28, 0x70, 0xe0, 0x7b, 0x9d, 0x85);

static constexpr UUID MSG_FOUND_CONTROLLER(
    0x6e, 0x52, 0xbe, 0x26, 0x46, 0xc4, 0x48, 0x39,
    0x88, 0x39, 0xa8, 0x63, 0x19, 0xa4, 0xfd, 0xbf);

}
}
