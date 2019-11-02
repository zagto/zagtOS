#include <common/common.hpp>
#include <tasks/UUID.hpp>

UUID::UUID(uint8_t id[16]) {
    memcpy(data, id, 16);
}

UUID::UUID() {
    memset(data, 0, 16);
}
