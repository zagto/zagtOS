#ifndef UUID_HPP
#define UUID_HPP

#include <common/inttypes.hpp>

class UUID {
private:
    uint8_t data[16];

public:
    UUID();
};

#endif // UUID_HPP
