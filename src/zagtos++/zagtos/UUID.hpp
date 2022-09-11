#pragma once

#include <cstdint>
#include <array>
#include <zagtos/ZBON.hpp>


namespace zagtos {
    class UUID {
    public:
        std::array<uint8_t, 16> data{0};

        constexpr UUID() {}
        constexpr UUID(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h,
             uint8_t i, uint8_t j, uint8_t k, uint8_t l ,uint8_t m, uint8_t n, uint8_t o, uint8_t p):
            data{a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p} {}
        UUID(const unsigned char *other) {
            for (size_t index = 0; index < 16; index++) {
                data[index] = other[index];
            }
        }

        bool operator==(UUID other) const {
            return data == other.data;
        }
        bool operator!=(UUID other) const {
            return data != other.data;
        }

        ZBON_ENCODING_FUNCTIONS(data);
    };

    std::ostream &operator<<(std::ostream &stream, UUID uuid);
}
