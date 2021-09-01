#include <log/Logger.hpp>
#include <Output.hpp>

Logger cout;

Logger &Logger::operator<<(char character) {
    if (character >= ' ' || character == '\n') {
        Output(character);
    }
    return *this;
}

Logger &Logger::operator<<(const char *string) {
    while (*string != '\0') {
        *this << *string;
        string++;
    }
    return *this;
}

Logger &Logger::operator<<(size_t value) {
    *this << "0x";
    for (int shift = sizeof(size_t) * 2 - 1; shift >= 0; shift--) {
        size_t part = (value >> (shift * 4)) & 0xf;
        if (part < 10) {
            *this << static_cast<char>(part + '0');
        } else {
            *this << static_cast<char>(part - 10 + 'a');
        }
    }
    return *this;
}

Logger &Logger::operator<<(uint32_t value) {
    *this << "0x";
    for (int shift = sizeof(uint32_t) * 2 - 1; shift >= 0; shift--) {
        size_t part = (value >> (shift * 4)) & 0xf;
        if (part < 10) {
            *this << static_cast<char>(part + '0');
        } else {
            *this << static_cast<char>(part - 10 + 'a');
        }
    }
    return *this;
}
