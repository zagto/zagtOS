#pragma once

#include <common/common.hpp>
#ifndef ZAGTOS_LOADER
#include <lib/Status.hpp>
#endif

namespace portio {

extern "C" uint8_t InB(uint16_t port);
extern "C" void OutB(uint16_t port, uint8_t data);
extern "C" uint16_t InW(uint16_t port);
extern "C" void OutW(uint16_t port, uint16_t data);
extern "C" uint32_t InD(uint16_t port);
extern "C" void OutD(uint16_t port, uint32_t data);

#ifndef ZAGTOS_LOADER
Result<size_t> read(uint16_t port, size_t size);
Status write(uint16_t port, size_t size, size_t data);
#endif

inline void waitIO() {
    /*
     * Explanation:
     * https://wiki.osdev.org/Inline_Assembly/Examples#IO_WAIT
     */
    OutB(0x80, 0);
}

}

