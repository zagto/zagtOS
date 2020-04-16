#include <common/common.hpp>
#include <portio.hpp>

namespace portio {

uint32_t read(uint16_t port, size_t size) {
    switch (size) {
    case 1:
        return InB(port);
    case 2:
        return InW(port);
    case 4:
        return InD(port);
    default:
        // TODO: throw user input
        assert(false);
        Panic();
    }
}

void write(uint16_t port, size_t size, uint32_t data) {
    switch (size) {
    case 1:
        OutB(port, static_cast<uint8_t>(data));
        return;
    case 2:
        OutW(port, static_cast<uint16_t>(data));
        return;
    case 4:
        OutD(port, data);
        return;
    default:
        // TODO: throw user input
        assert(false);
    }
}

}
