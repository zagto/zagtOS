#include <common/common.hpp>
#include <processes/UUID.hpp>

UUID::UUID(const uint8_t id[16]) {
    memcpy(data, id, 16);
}

UUID::UUID() {
    memset(data, 0, 16);
}
