#include <zagtos/IOPortRange.hpp>
#include <zagtos/syscall.h>
#include <memory>

namespace zagtos {

IOPortRange::IOPortRange(uint16_t start, uint16_t length) {
    _handle = zagtos_syscall2(SYS_CREATE_IO_PORT_RANGE, start, length);
}

IOPortRange &IOPortRange::operator=(IOPortRange &&other) {
    HandleObject::operator=(std::move(other));
    return *this;
}

uint32_t IOPortRange::read(uint16_t offset, size_t size) {
    return zagtos_syscall3(SYS_IO_PORT_READ, _handle, offset, size);

}

void IOPortRange::write(uint16_t offset, size_t size, uint32_t value) {
    zagtos_syscall4(SYS_IO_PORT_WRITE, _handle, offset, size, value);
}

}
