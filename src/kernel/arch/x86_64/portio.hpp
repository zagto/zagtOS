#ifndef PORTIO_HPP
#define PORTIO_HPP

#include <common/common.hpp>

namespace portIO {
    extern "C" u8 InB(u16 port);
    extern "C" void OutB(u16 port, u8 data);
}

#endif // PORTIO_HPP
