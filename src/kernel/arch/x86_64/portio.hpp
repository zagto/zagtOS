#ifndef PORTIO_HPP
#define PORTIO_HPP

#include <common/common.hpp>

namespace portIO {
    extern "C" uint8_t InB(uint16_t port);
    extern "C" void OutB(uint16_t port, uint8_t data);
}

#endif // PORTIO_HPP
