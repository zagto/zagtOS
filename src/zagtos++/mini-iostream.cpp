#include <zagtos/mini-iostream.hpp>
#include <unistd.h>

namespace mio {

OutputStream cout;
OutputStream cerr;

static void printString(const char* string, size_t length) {
    write(0, string, length);

}

static void printHex(size_t value) {
    char buffer[2 + sizeof(size_t) * 2];
    buffer[0] = '0';
    buffer[1] = 'x';
    for (size_t shift = 0; shift < sizeof(size_t)*2; shift++) {
        size_t part = (value >> (shift * 4)) & 0xf;
        if (part < 10) {
            buffer[2+shift] = static_cast<char>(part + '0');
        } else {
            buffer[2+shift] = static_cast<char>(part - 10 + 'a');
        }
    }
    printString(buffer, sizeof(buffer));
}

static void printUnsigned(uint64_t value) {
    static constexpr size_t bufferSize = sizeof(uint64_t) * 3 + 1;
    char buffer[bufferSize];
    size_t position = bufferSize;
    do {
        position--;
        size_t part = value % 10;
        buffer[position] = static_cast<char>(part + '0');

        value = value / 10;
    } while (value != 0);

    printString(&buffer[position], bufferSize - position);
}

/*static void printSigned(int64_t value) {
    static constexpr size_t bufferSize = sizeof(int64_t) * 3 + 1;
    char buffer[bufferSize];
    size_t position = bufferSize;
    bool negative = false;
    if (value < 0) {
        negative = true;
        value = value * (-1);
    }
    do {
        position--;
        size_t part = value % 10;
        buffer[position] = static_cast<char>(part + '0');

        value = value / 10;
    } while (value != 0);
    if (negative) {
        position--;
        buffer[position] = '-';
    }

    printString(&buffer[position], bufferSize - position);
}*/

OutputStream &OutputStream::operator<<(const char *string) {
    size_t length = 0;
    while (string[length] != '\0') {
        length++;
    }
    printString(string, length);
    return *this;
}

OutputStream &OutputStream::operator<<(uint64_t value) {
    printUnsigned(value);
    return *this;
}

OutputStream &OutputStream::operator<<(uint32_t value) {
    printUnsigned(value);
    return *this;
}

OutputStream &OutputStream::operator<<(uint16_t value) {
    printUnsigned(value);
    return *this;
}

OutputStream &OutputStream::operator<<(char value) {
    printString(&value, 1);
    return *this;
}

OutputStream &OutputStream::operator<<(bool value) {
    if (value) {
        printString("true", 4);
    } else {
        printString("false", 5);
    }
    return *this;
}

OutputStream &OutputStream::operator<<(volatile void *pointer) {
    printHex(reinterpret_cast<size_t>(pointer));
    return *this;
}

OutputStream &OutputStream::operator<<(const void *pointer) {
    printHex(reinterpret_cast<size_t>(pointer));
    return *this;
}

OutputStream &OutputStream::operator<<(void *pointer) {
    printHex(reinterpret_cast<size_t>(pointer));
    return *this;
}

}
