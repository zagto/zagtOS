#pragma once
#include <common/inttypes.hpp>

class UUID {
private:
    uint8_t data[16];

public:
    UUID(const uint8_t *id);
    constexpr UUID(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h,
         uint8_t i, uint8_t j, uint8_t k, uint8_t l ,uint8_t m, uint8_t n, uint8_t o, uint8_t p):
        data{a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p} {}
    UUID() = default;
};
