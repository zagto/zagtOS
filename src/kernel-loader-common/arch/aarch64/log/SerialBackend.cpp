#include <log/SerialBackend.hpp>
#include <log/BasicLog.hpp>

static const uint16_t SERIAL_PORT = 0x3f8;


void SerialBackend::init() {
    // TODO: not yet implemented
}

bool SerialBackend::isTransmitEmpty() {
    // TODO: not yet implemented
    return true;
}

void SerialBackend::writeCharacter(char /*character*/) {
    // TODO: not yet implemented
}

void SerialBackend::setKernelColor() {
    write(27);
    write('[');
    write('3');
    write('4');
    write('m');
}

void SerialBackend::setProgramNameColor() {
    write(27);
    write('[');
    write('3');
    write('1');
    write('m');
}

void SerialBackend::setProgramColor() {
    write(27);
    write('[');
    write('3');
    write('0');
    write('m');
}

void SerialBackend::write(char character) {
    switch (character) {
    case basicLog::ControlCharacter::KERNEL_COLOR:
        setKernelColor();
        break;
    case basicLog::ControlCharacter::PROGRAM_NAME_COLOR:
        setProgramNameColor();
        break;
    case basicLog::ControlCharacter::PROGRAM_COLOR:
        setProgramColor();
        break;
    default:
        writeCharacter(character);
    }
}
