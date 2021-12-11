#include <common/common.hpp>
#include <portio.hpp>
#include <lib/Exception.hpp>
#include <processes/Process.hpp>

namespace portio {

size_t read(uint16_t port, size_t size) {
    switch (size) {
    case 1:
        return InB(port);
    case 2:
        return InW(port);
    case 4:
        return InD(port);
    default:
        throw BadUserSpace(CurrentProcess());
    }
}

void write(uint16_t port, size_t size, size_t data) {
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
        throw BadUserSpace(CurrentProcess());
    }
}

}
