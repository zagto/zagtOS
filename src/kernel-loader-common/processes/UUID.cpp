#include <common/common.hpp>
#include <processes/UUID.hpp>

UUID::UUID(const uint8_t *id) {
    if (id == nullptr) {
        memset(data, 0, 16);
    } else {
        memcpy(data, id, 16);
    }
}

UUID::UUID() {
    memset(data, 0, 16);
}
