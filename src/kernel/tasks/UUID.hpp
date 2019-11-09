#ifndef UUID_HPP
#define UUID_HPP

#include <common/inttypes.hpp>

class UUID {
private:
    uint8_t data[16];

public:
    UUID(const uint8_t id[]);
    UUID();
};

#endif // UUID_HPP
