#include <common/common.hpp>
#include <portio.hpp>

namespace portio {

Result<size_t> read(uint16_t port, size_t size) {
    switch (size) {
    case 1:
        return InB(port);
    case 2:
        return InW(port);
    case 4:
        return InD(port);
    default:
        return Status::BadUserSpace();
    }
}

Status write(uint16_t port, size_t size, size_t data) {
    switch (size) {
    case 1:
        OutB(port, static_cast<uint8_t>(data));
        return Status::OK();
    case 2:
        OutW(port, static_cast<uint16_t>(data));
        return Status::OK();
    case 4:
        OutD(port, data);
        return Status::OK();
    default:
        return Status::BadUserSpace();
    }
}

}
