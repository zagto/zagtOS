#include <assert.h>
#include <zagtos/syscall.h>
#include <zagtos/PortIO.h>

uint32_t ZoReadPort(uint16_t address, size_t length) {
    assert(length == 1 || length == 2 || length == 4);
    return (uint32_t)zagtos_syscall(SYS_IO_PORT_READ, address, length);
}

void ZoWritePort(uint16_t address, size_t length, uint32_t value) {
    assert(length == 1 || length == 2 || length == 4);
    zagtos_syscall(SYS_IO_PORT_WRITE, address, length, (uint64_t)value);
}
