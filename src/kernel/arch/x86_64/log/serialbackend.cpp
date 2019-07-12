#include <common/common.hpp>
#include <log/serialbackend.hpp>
#include <portio.hpp>

using namespace log;


static const u16 SERIAL_PORT = 0x3f8;


void SerialBackend::init() {
    /* In case the system was booted by a non-debug bootloader, the serial port may not be
     * initialized yet. Do it now!
     * based on: https://wiki.osdev.org/Serial_Ports */

    // disable interrupts
    portIO::OutB(SERIAL_PORT + 1, 0);

    // enable DLAB
    portIO::OutB(SERIAL_PORT + 3, 0x80);

    // set divisor to 3 (low + high byte)
    portIO::OutB(SERIAL_PORT, 3);
    portIO::OutB(SERIAL_PORT + 1, 0);

    // 8 bits, no parity, 1 stop bit
    portIO::OutB(SERIAL_PORT + 3, 0x03);

    // Enable FIFO, clear them, with 14-byte threshold
    portIO::OutB(SERIAL_PORT + 2, 0xc7);

    // IRQs enabled, RTS/DSR set
    portIO::OutB(SERIAL_PORT + 4, 0x0b);
}


bool SerialBackend::isTransmitEmpty() {
    return portIO::InB(SERIAL_PORT + 5) & 0x20;
}


void SerialBackend::write(char character) {
    while (!isTransmitEmpty());
    portIO::OutB(SERIAL_PORT, static_cast<u8>(character));
}
