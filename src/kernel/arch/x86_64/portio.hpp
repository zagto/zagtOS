#ifndef PORTIO_HPP
#define PORTIO_HPP

#include <common/common.hpp>

extern "C" uint8_t InB(uint16_t port);
extern "C" void OutB(uint16_t port, uint8_t data);

inline void waitIO() {
    /*
     * Explanation:
     * https://wiki.osdev.org/Inline_Assembly/Examples#IO_WAIT
     */
    OutB(0x80, 0);
}

#endif // PORTIO_HPP
