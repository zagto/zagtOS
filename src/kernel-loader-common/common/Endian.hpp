#pragma once
#include <common/inttypes.hpp>

template<typename T>
T ByteSwap(T input) {
    T output;
    const uint8_t *inputBytes = reinterpret_cast<const uint8_t *>(&input);
    uint8_t *outputBytes = reinterpret_cast<uint8_t *>(&output);
    for (size_t byteIndex = 0; byteIndex < sizeof(T); byteIndex++) {
        outputBytes[byteIndex] = inputBytes[sizeof(T) - byteIndex - 1];
    }
    return output;
}

template<typename T>
struct BigEndian {
    T bigEndianValue;
    BigEndian() = default;
    BigEndian(T platformValue) {
#ifdef PLATFORM_LITTLE_ENDIAN
        bigEndianValue = ByteSwap(platformValue);
#else
#error "Unsupported Endianness"
#endif
    }
    operator T() const {
#ifdef PLATFORM_LITTLE_ENDIAN
        return ByteSwap(bigEndianValue);
#else
#error "Unsupported Endianness"
#endif
    }
    bool operator==(const BigEndian &other) const {
        return bigEndianValue == other.bigEndianValue;
    }
    bool operator!=(const BigEndian &other) const {
        return bigEndianValue != other.bigEndianValue;
    }
    bool operator==(const T &other) const {
        return static_cast<T>(*this) == other;
    }
    bool operator!=(const T &other) const {
        return static_cast<T>(*this) != other;
    }
};

template<typename T>
struct LittleEndian {
    T littleEndianValue;
    LittleEndian() = default;
    LittleEndian(T platformValue) {
#ifdef PLATFORM_LITTLE_ENDIAN
        littleEndianValue = ByteSwap(platformValue);
#else
#error "Unsupported Endianness"
#endif
    }
    operator T() const {
#ifdef PLATFORM_LITTLE_ENDIAN
        return littleEndianValue;
#else
#error "Unsupported Endianness"
#endif
    }
    bool operator==(const LittleEndian &other) const {
        return littleEndianValue == other.bigEndianValue;
    }
    bool operator!=(const LittleEndian &other) const {
        return littleEndianValue != other.bigEndianValue;
    }
    bool operator==(const T &other) const {
        return static_cast<T>(*this) == other;
    }
    bool operator!=(const T &other) const {
        return static_cast<T>(*this) != other;
    }
};
