#include <common/inttypes.hpp>
#include <log/SerialBackend.hpp>
#include <log/BasicLog.hpp>

static const uint16_t SERIAL_PORT = 0x3f8;


void SerialBackend::init(const hos_v1::SerialInfo &serialInfo) {
    info = serialInfo;

    switch (info.type) {
    case hos_v1::SerialType::PC:
#ifdef SYSTEM_X86_64
        pcSerial.init();
#endif
        break;
    }
    // reset terminal text attributes
    write(27);
    write('[');
    write('m');
}

void SerialBackend::writeCharacter(char character) {
    switch (info.type) {
    case hos_v1::SerialType::PC:
#ifdef SYSTEM_X86_64
        pcSerial.writeCharacter(character);
#endif
        break;
    case hos_v1::SerialType::PRIMECELL_PL011:
        *reinterpret_cast<volatile char *>(info.baseAddress) = character;
        break;
    }
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
