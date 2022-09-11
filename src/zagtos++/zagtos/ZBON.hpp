#pragma once

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <vector>
#include <array>
#include <tuple>
#include <optional>
#include <limits>
#include <iostream>

#ifdef __zagtos__
/* MessageData type */
#include <zagtos/MessageData.hpp>
namespace zbon {
using EncodedData = zagtos::MessageData;
}
#else
namespace zbon {
struct EncodedData {
    uint8_t *data;
    size_t size;
    size_t numHandles;
    bool allocatedExternally;

    constexpr EncodedData(const uint8_t *data,
                          size_t size,
                          size_t numHandles,
                          bool allocatedExternally = false) :
        data{data},
        size{size},
        numHandles{numHandles},
        allocatedExternally{allocatedExternally} {}
    EncodedData(const MessageData &other) = delete;
    EncodedData(MessageData &&other) = default;
    ~EncodedData() {
        if (data != nullptr && !allocatedExternally) {
            delete[] data;
        }
    }
};
}
#endif

namespace zbon {

enum class Type : uint8_t {
    BINARY = 1, NOTHING, OBJECT, STRING, BOOLEAN,
    INT8, UINT8, INT16, UINT16, INT32, UINT32, INT64, UINT64,
    FLOAT, DOUBLE, HANDLE, TYPES_END
};
static const size_t HANDLE_SIZE = 4;

struct Size {
    size_t numRegularBytes{0};
    size_t numHandles{0};

    Size operator+(const Size &b) const {
        return {numRegularBytes + b.numRegularBytes, numHandles + b.numHandles};
    }
    Size operator+=(const Size &b) {
        numRegularBytes += b.numRegularBytes;
        numHandles += b.numHandles;
        return *this;
    }
};


std::ostream &operator<<(std::ostream &stream, Type type);

template<typename T>
static Type typeFor(const T& object) {
    return object.ZBONType();
}

static constexpr Type typeFor(bool) {
    return Type::BOOLEAN;
}
static constexpr Type typeFor(char) {
    return Type::INT8;
}
static constexpr Type typeFor(int8_t) {
    return Type::INT8;
}
static constexpr Type typeFor(uint8_t) {
    return Type::UINT8;
}
static constexpr Type typeFor(int16_t) {
    return Type::INT16;
}
static constexpr Type typeFor(uint16_t) {
    return Type::UINT16;
}
static constexpr Type typeFor(int32_t) {
    return Type::INT32;
}
static constexpr Type typeFor(uint32_t) {
    return Type::UINT32;
}
static constexpr Type typeFor(int64_t) {
    return Type::INT64;
}
static constexpr Type typeFor(uint64_t) {
    return Type::UINT64;
}
static constexpr Type typeFor(double) {
    return Type::DOUBLE;
}
static constexpr Type typeFor(float) {
    return Type::FLOAT;
}
template<typename T>
static Type typeFor(const std::vector<T> &) {
    return Type::OBJECT;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static Type typeFor(const std::vector<uint8_t> &) {
    return Type::BINARY;
}
#pragma GCC diagnostic pop
template<typename T, size_t count>
static Type typeFor(const std::array<T, count> &) {
    return Type::OBJECT;
}
template<typename T, size_t count>
static Type typeFor(const T (&)[count]) {
    return Type::OBJECT;
}
template<size_t count>
static Type typeFor(const uint8_t (&)[count]) {
    return Type::BINARY;
}
template<typename ...Types>
static Type typeFor(const std::tuple<Types...> &) {
    return Type::OBJECT;
}
template<typename T>
static Type typeFor(const std::optional<T> &option) {
    if (option) {
        return typeFor(*option);
    } else {
        return Type::NOTHING;
    }
}

#define INSERT_NUMBER_TYPES \
NUMBER_TYPE(char)  \
NUMBER_TYPE(uint8_t)  \
NUMBER_TYPE (int8_t)  \
NUMBER_TYPE(uint16_t) \
NUMBER_TYPE( int16_t) \
NUMBER_TYPE(uint32_t) \
NUMBER_TYPE( int32_t) \
NUMBER_TYPE(uint64_t) \
NUMBER_TYPE( int64_t) \
NUMBER_TYPE(float)    \
NUMBER_TYPE(double)


static const size_t COUNT_SIZE = 8;
static const size_t TYPE_SIZE = 1;
static const size_t HEADER_SIZE = TYPE_SIZE + COUNT_SIZE * 3;


template<typename T>
static Size sizeFor(const T& object) {
    return object.ZBONSize();
}

static constexpr Size sizeFor(bool) {
    return {1 + TYPE_SIZE};
}
Size sizeFor(std::string string);
#define NUMBER_TYPE(T) constexpr Size sizeFor(const T) { return {sizeof(T) + TYPE_SIZE}; }
INSERT_NUMBER_TYPES
#undef NUMBER_TYPE

/* The following versions of sizeFor may use others internally. forward-declare them to make
 * sure everything is avalable */
template<typename T>
static Size sizeFor(const std::vector<T> &vector);
template<typename T, size_t count>
static Size sizeFor(const std::array<T, count> &array);
template<typename T, size_t count>
static Size sizeFor(const T (&array)[count]);
template<typename ...Types>
static Size sizeFor(const std::tuple<Types...> &tuple);
template<typename T>
static Size sizeFor(const std::optional<T> &option);
static Size sizeForBinary(size_t length);

template<typename T>
static Size sizeFor(const std::vector<T> &vector) {
    Size sum = {HEADER_SIZE};
    for (const T &element: vector) {
        sum += sizeFor(element);
    }
    return sum;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static Size sizeFor(const std::vector<uint8_t> &vector) {
    return sizeForBinary(vector.size());
}
#pragma GCC diagnostic pop
template<typename T, size_t count>
static Size sizeFor(const std::array<T, count> &array) {
    Size sum = {HEADER_SIZE};
    for (const T &element: array) {
        sum += sizeFor(element);
    }
    return sum;
}
template<size_t count>
static Size sizeFor(const uint8_t (&array)[count]) {
    /* uint8_t array becomes binary */
    return {TYPE_SIZE + COUNT_SIZE + count};
}
template<typename T, size_t count>
static Size sizeFor(const T (&array)[count]) {
    Size sum = {HEADER_SIZE};
    for (const T &element: array) {
        sum += sizeFor(element);
    }
    return sum;
}
template<typename T>
static Size sizeForArray(const T *array, size_t numElements) {
    Size sum = {HEADER_SIZE};
    for (size_t i = 0; i < numElements; i++) {
        sum += sizeFor(array[i]);
    }
    return sum;
}
template<size_t position, typename ...Types>
static Size sizeForTupleElements(const std::tuple<Types...> &tuple) {
    if constexpr(position == std::tuple_size<std::tuple<Types...>>::value) {
        return {};
    } else {
        const auto &element = std::get<position>(tuple);
        return sizeFor(element) + sizeForTupleElements<position + 1>(tuple);
    }
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static Size sizeForObjectElements() {
    return Size{0};
}
#pragma GCC diagnostic pop
template<typename HeadType, typename ...TailTypes>
static Size sizeForObjectElements(const HeadType &head, const TailTypes &...tail) {
    return sizeFor(head) + sizeForObjectElements(tail...);
}
template<typename ...Types>
static Size sizeForObject(const Types &...elements) {
    return sizeForObjectElements<Types...>(elements...) + Size{HEADER_SIZE};
}
template<typename ...Types>
static Size sizeFor(const std::tuple<Types...> &tuple) {
    return sizeForTupleElements<0, Types...>(tuple) + Size{HEADER_SIZE};
}
template<typename T>
static Size sizeFor(const std::optional<T> &option) {
    if (option) {
        return sizeFor(*option);
    } else {
        return {TYPE_SIZE};
    }
}
static Size sizeForBinary(size_t length) {
    return {length + COUNT_SIZE + TYPE_SIZE};
}


void copyConvertEndianness(void *destination, const void *source, size_t length);

class DecoderException : public std::exception {
private:
    virtual const char* what() const throw() {
        return "ZBON Decoder got invalid data";
    }
};

class Encoder {
private:
    uint8_t *data;
    size_t position;
    size_t handlePosition;

    void encodeType(Type type) {
        data[position] = static_cast<uint8_t>(type);
        position += TYPE_SIZE;
    }

    /* The bytesSize and numHanldes fields are filled out when the end of the element is reached so
     * the value is known. Other than that it's just a 64-bit unsigned number */
    void encodeSize(size_t value, size_t bytesSizePosition) {
        copyConvertEndianness(&data[bytesSizePosition], &value, COUNT_SIZE);
    }

    template<typename T>
    void encodeArray(const T &array, size_t length) {
        encodeType(Type::OBJECT);
        encodeNumber(static_cast<uint64_t>(length), position);

        /* insert numHandles (64bit) later */
        size_t numHandlesPosition = position;
        size_t handlePositionStart = handlePosition;
        position += COUNT_SIZE;

        /* insert bytesSize (64bit) later */
        size_t bytesSizePosition = position;
        position += COUNT_SIZE;

        for (size_t i = 0; i < length; i++) {
            encodeValue(array[i]);
        }

        size_t bytesSize = position - bytesSizePosition - COUNT_SIZE;
        encodeSize(bytesSize, bytesSizePosition);

        size_t numHandles = (handlePosition - handlePositionStart) / HANDLE_SIZE;
        encodeSize(numHandles, numHandlesPosition);
    }

    template<typename T>
    void encodeNumber(T value, size_t &numberPosition) {
        copyConvertEndianness(&data[numberPosition], &value, sizeof(T));
        numberPosition += sizeof(T);
    }

    template<size_t position, typename ...Types>
    void encodeTupleElements(const std::tuple<Types...> &tuple) {
        if constexpr(position == std::tuple_size<std::tuple<Types...>>::value) {
            return;
        } else {
            const auto &element = std::get<position>(tuple);
            encodeValue(element);
            encodeTupleElements<position + 1, Types...>(tuple);
        }
    }
    void encodeObjectElements() {
        /* nothing to do */
    }
    template<typename HeadType, typename ...TailTypes>
    void encodeObjectElements(const HeadType &head, const TailTypes &...tail) {
        encodeValue(head);
        encodeObjectElements(tail...);
    }

public:
    void encodeValue(bool value) {
        encodeType(Type::BOOLEAN);
        data[position] = value;
        position += 1;
    }
#define NUMBER_TYPE(T) void encodeValue(T value) { \
        encodeType(typeFor(value)); \
        encodeNumber(value, position); \
    }
    INSERT_NUMBER_TYPES
#undef NUMBER_TYPE
    template<typename T>
    void encodeValue(const std::vector<T> &vector) {
        encodeArray(vector, vector.size());
    }
    void encodeValue(const std::vector<uint8_t> &vector) {
        encodeBinary(vector.data(), vector.size());
    }
    template<typename T, size_t count>
    void encodeValue(const std::array<T, count> &array) {
        encodeArray(array, count);
    }
    template<typename T, size_t count>
    void encodeValue(const T (&array)[count]) {
        encodeArray(array, count);
    }
    void encodeValue(std::string string) {
        encodeType(Type::STRING);
        encodeNumber(static_cast<uint64_t>(string.size()), position);
        for (size_t i = 0; i < string.size(); i++) {
            data[position] = string[i];
            position++;
        }
    }
    template<typename ...Types>
    void encodeValue(const std::tuple<Types...> &tuple) {
        encodeType(Type::OBJECT);
        encodeNumber(static_cast<uint64_t>(sizeof...(Types)), position);

        /* insert numHandles (64bit) later */
        size_t numHandlesPosition = position;
        size_t handlePositionStart = handlePosition;
        position += COUNT_SIZE;

        /* insert bytesSize (64bit) later */
        size_t bytesSizePosition = position;
        position += COUNT_SIZE;

        encodeTupleElements<0, Types...>(tuple);

        size_t bytesSize = position - bytesSizePosition - COUNT_SIZE;
        encodeSize(bytesSize, bytesSizePosition);

        size_t numHandles = (handlePosition - handlePositionStart) / HANDLE_SIZE;
        encodeSize(numHandles, numHandlesPosition);
    }
    template<typename ...Types>
    void encodeObjectValue(const Types &...values) {
        encodeType(Type::OBJECT);
        encodeNumber(static_cast<uint64_t>(sizeof...(Types)), position);

        /* insert numHandles (64bit) later */
        size_t numHandlesPosition = position;
        size_t handlePositionStart = handlePosition;
        position += COUNT_SIZE;

        /* insert bytesSize (64bit) later */
        size_t bytesSizePosition = position;
        position += COUNT_SIZE;

        encodeObjectElements(values...);

        size_t bytesSize = position - bytesSizePosition - COUNT_SIZE;
        encodeSize(bytesSize, bytesSizePosition);

        size_t numHandles = (handlePosition - handlePositionStart) / HANDLE_SIZE;
        encodeSize(numHandles, numHandlesPosition);
    }
    template<typename T>
    void encodeValue(const T& object) {
        object.ZBONEncode(*this);
    }
    template<typename T>
    void encodeValue(const std::optional<T> &option) {
        if (option) {
            encodeValue(*option);
        } else {
            encodeType(Type::NOTHING);
        }
    }
    void encodeValue(const EncodedData &value);

    void encodeHandle(const uint32_t handle) {
        encodeType(Type::HANDLE);
        encodeNumber(handle, handlePosition);
    }
    void encodeBinary(const uint8_t *value, size_t length);

    Encoder():
        data{nullptr},
        position{0},
        handlePosition{0} {}

    template<typename T>
    EncodedData encode(const T &cppData) {
        /* the root element is encoded like an object property (with type byte), so the type of
         * the whole thing is known on decoding */
        Size size = sizeFor(cppData);
        size_t handlesSize = size.numHandles * HANDLE_SIZE;
        size_t bytesSize = size.numRegularBytes + handlesSize;

        data = new uint8_t[bytesSize];
        EncodedData encodedData(data, bytesSize, size.numHandles);
        assert(position == 0);
        assert(handlePosition == 0);
        position = handlesSize;

        encodeValue(cppData);

        /* check encoding actually used the amount of space the size calculation got */
        assert(position == bytesSize);
        assert(handlePosition == handlesSize);
        return encodedData;
    }

    template<typename ...Types>
    EncodedData encodeObject(const Types &...cppData) {
        /* the root element is encoded like an object property (with type byte), so the type of
         * the whole thing is known on decoding */
        Size size = sizeForObject(cppData...);
        size_t handlesSize = size.numHandles * HANDLE_SIZE;
        size_t bytesSize = size.numRegularBytes + handlesSize;

        data = new uint8_t[bytesSize];
        EncodedData encodedData(data, bytesSize, size.numHandles);
        assert(position == 0);
        assert(handlePosition == 0);
        position = handlesSize;

        encodeObjectValue(cppData...);

        /* check encoding actually used the amount of space the size calculation got */
        assert(position == bytesSize);
        assert(handlePosition == handlesSize);
        return encodedData;
    }
};

class Decoder {
private:
    const EncodedData &encodedData;
    size_t position;
    size_t handlePosition;

    void ensureEnoughLeft(size_t numBytes) {
        if (!(position < encodedData.size && encodedData.size - position >= numBytes)) {
            std::cerr << "ZBON: Unexpected End of Data at " << encodedData.size << std::endl;
            throw DecoderException();
        }
    }

    void ensureEnoughHandlesLeft(uint64_t numHandles) {
        if (!(handlePosition / HANDLE_SIZE + numHandles <= encodedData.numHandles
              && handlePosition / HANDLE_SIZE + numHandles >= numHandles)) {
            std::cerr << "ZBON: Unexpectedly lagre amount of handles" << std::endl;
            throw DecoderException();
        }
    }

    void decodeType(Type &result) {
        ensureEnoughLeft(1);

        uint8_t value = encodedData.data[position];
        if (value > 0 && value < static_cast<uint8_t>(Type::TYPES_END)) {
            result = static_cast<Type>(value);
            position++;
        } else {
            std::cerr << "ZBON: Invalid Type Byte at " << position << std::endl;
            throw DecoderException();
        }
    }

    template<typename T>
    void decodeNumber(T &result, size_t &position) {
        ensureEnoughLeft(sizeof(T));
        copyConvertEndianness(&result, &encodedData.data[position], sizeof(T));
        position += sizeof(T);
    }

    void decodeVerifyType(Type elementType) {
        Type encodedType;
        decodeType(encodedType);
        if (elementType != encodedType) {
            std::cerr << "ZBON: program expects a " << elementType
                      << " but ZBON data holds " << encodedType << std::endl;
            throw DecoderException();
        }
    }

    template<size_t position, typename ...Types>
    void decodeTupleElements(std::tuple<Types...> &tuple) {
        if constexpr(position == std::tuple_size<std::tuple<Types...>>::value) {
            return;
        } else {
            auto &element = std::get<position>(tuple);
            decodeValue(element);
            return decodeTupleElements<position + 1, Types...>(tuple);
        }
    }

    void decodeFromObjectElements() {
        /* do nothing */
    }
    template<typename HeadType, typename ...TailTypes>
    void decodeFromObjectElements(HeadType &head, TailTypes &...tail) {
        decodeValue(head);
        return decodeFromObjectElements(tail...);
    }

public:
    template<typename ...Types>
    void decodeValue(std::tuple<Types...> &tuple) {
        decodeVerifyType(Type::OBJECT);

        uint64_t numElements;
        decodeNumber(numElements, position);
        if (numElements != sizeof...(Types)) {
            std::cerr << "ZBON: requested decode of Tuple of " << sizeof...(Types)
                      << " elements but ZBON Object holds " << numElements << " elements."
                      << std::endl;
            throw DecoderException();
        }

        size_t handlePositionStart = handlePosition;
        uint64_t numHandles;
        decodeNumber(numHandles, position);

        uint64_t bytesSize;
        decodeNumber(bytesSize, position);
        size_t positionBeforeData = position;

        decodeTupleElements<0, Types...>(tuple);

        if (position - positionBeforeData != bytesSize) {
            std::cerr << "ZBON: Object array has bytesSize value of " << bytesSize
                      << " but is actually " << (positionBeforeData - position) << " bytes big."
                      << std::endl;
            throw DecoderException();
        }

        if ((handlePosition - handlePositionStart) / HANDLE_SIZE != numHandles) {
            std::cerr << "ZBON: Object array has numHandles value of " << numHandles
                      << " but actually has "
                      << ((handlePosition - handlePositionStart) / HANDLE_SIZE) << " handles."
                      << std::endl;
            throw DecoderException();
        }
    }

    /* decodes the same values as the above, but for applications that don't want to keep the data
     * as tuple */
    template<typename ...Types>
    void decodeFromObject(Types &...result) {
        decodeVerifyType(Type::OBJECT);

        uint64_t numElements;
        decodeNumber(numElements, position);
        if (numElements != sizeof...(Types)) {
            std::cerr << "ZBON: requested decode of Tuple of " << sizeof...(Types)
                      << " elements but ZBON Object holds " << numElements << " elements."
                      << std::endl;
            throw DecoderException();
        }

        size_t handlePositionStart = handlePosition;
        uint64_t numHandles;
        decodeNumber(numHandles, position);

        uint64_t bytesSize;
        decodeNumber(bytesSize, position);
        size_t positionBeforeData = position;

        decodeFromObjectElements(result...);

        if (position - positionBeforeData != bytesSize) {
            std::cerr << "ZBON: Object array has bytesSize value of " << bytesSize
                      << " but is actually " << (positionBeforeData - position) << " bytes big."
                      << std::endl;
            throw DecoderException();
        }

        if ((handlePosition - handlePositionStart) / HANDLE_SIZE != numHandles) {
            std::cerr << "ZBON: Object array has numHandles value of " << numHandles
                      << " but actually has "
                      << ((handlePosition - handlePositionStart) / HANDLE_SIZE) << " handles."
                      << std::endl;
            throw DecoderException();
        }
    }

    void decodeValue(bool &result) {
        decodeVerifyType(Type::BOOLEAN);
        ensureEnoughLeft(1);

        result = static_cast<bool>(encodedData.data[position]);
        position++;
    }

#define NUMBER_TYPE(T) void decodeValue(T &result) { \
    decodeVerifyType(typeFor(result)); \
    return decodeNumber(result, position); \
}
    INSERT_NUMBER_TYPES
#undef NUMBER_TYPE

    template<typename ArrayType, typename ElementType>
    void decodeArray(ArrayType &resultArray) {
        decodeVerifyType(Type::OBJECT);

        uint64_t numElements;
        decodeNumber(numElements, position);
        if (numElements != std::size(resultArray)) {
            std::cerr << "ZBON: requested decode of Array of " << std::size(resultArray)
                      << " elements but ZBON Array holds " << numElements << " elements."
                      << std::endl;
            throw DecoderException();
        }

        size_t handlePositionStart = handlePosition;
        uint64_t numHandles;
        decodeNumber(numHandles, position);

        uint64_t bytesSize;
        decodeNumber(bytesSize, position);
        size_t positionBeforeData = position;

        for (auto &element: resultArray) {
            decodeValue(element);
        }

        if (position - positionBeforeData != bytesSize) {
            std::cerr << "ZBON: Object array has bytesSize value of " << bytesSize
                      << " but is actually " << (positionBeforeData - position) << " bytes big."
                      << std::endl;
            throw DecoderException();
        }

        if ((handlePosition - handlePositionStart) / HANDLE_SIZE != numHandles) {
            std::cerr << "ZBON: Object array has numHandles value of " << numHandles
                      << " but actually has "
                      << ((handlePosition - handlePositionStart) / HANDLE_SIZE) << " handles."
                      << std::endl;
            throw DecoderException();
        }
    }

    void decodeHandle(uint32_t &result) {
        decodeVerifyType(Type::HANDLE);
        ensureEnoughHandlesLeft(1);
        copyConvertEndianness(&result, &encodedData.data[handlePosition], HANDLE_SIZE);
        handlePosition += HANDLE_SIZE;
    }

    template<typename T, std::size_t length>
    void decodeValue(std::array<T, length> &result) {
        decodeArray<std::array<T, length>, T>(result);
    }
    template<typename T, size_t length>
    void decodeValue(T (&result)[length]) {
        decodeArray<T[length], T>(result);
    }

    template<typename T>
    void decodeValue(std::vector<T> &result) {
        decodeVerifyType(Type::OBJECT);

        uint64_t numElements;
        decodeNumber(numElements, position);
        result.resize(numElements);

        size_t handlePositionStart = handlePosition;
        uint64_t numHandles;
        decodeNumber(numHandles, position);

        uint64_t bytesSize;
        decodeNumber(bytesSize, position);
        size_t positionBeforeData = position;

        for (auto &element: result) {
            decodeValue(element);
        }

        if (position - positionBeforeData != bytesSize) {
            std::cerr << "ZBON: Object array has bytesSize value of " << bytesSize
                      << " but is actually " << (positionBeforeData - position) << " bytes big."
                      << std::endl;
            throw DecoderException();
        }

        if ((handlePosition - handlePositionStart) / HANDLE_SIZE != numHandles) {
            std::cerr << "ZBON: Object array has numHandles value of " << numHandles
                      << " but actually has "
                      << ((handlePosition - handlePositionStart) / HANDLE_SIZE) << " handles."
                      << std::endl;
            throw DecoderException();
        }
    }
    void decodeValue(std::vector<uint8_t> &result) {
        size_t length = getBinaryLength();
        result.resize(length);
        decodeBinary(result.data(), result.size());
    }
    void decodeValue(std::string &result) {
        decodeVerifyType(Type::STRING);
        uint64_t numElements;
        decodeNumber(numElements, position);
        result.resize(numElements);

        ensureEnoughLeft(numElements);
        for (size_t index = 0; index < numElements; index++) {
            result[index] = encodedData.data[position + index];
        }
        position += numElements;
    }

    template<typename T>
    void decodeValue(std::optional<T> &option) {
        Type type;
        decodeType(type);
        if (type == Type::NOTHING) {
            option.reset();
        } else {
            position--;
            option.emplace();
            decodeValue(*option);
        }
    }

    void decodeValue(zbon::EncodedData &value);

    template<typename T>
    void decodeValue(T& object) {
         object.ZBONDecode(*this);
    }

    void decodeBinary(uint8_t *buffer, size_t length);
    uint64_t getBinaryLength();

    Decoder(const EncodedData &encodedData) :
        encodedData{encodedData},
        position{encodedData.numHandles * HANDLE_SIZE},
        handlePosition{0} {}

    template<typename T>
    void decode(T &result) {
        uint64_t handlesSize = encodedData.numHandles * HANDLE_SIZE;

        assert(position == handlesSize);
        decodeValue(result);

        if ((position != encodedData.size)
                || (handlePosition != handlesSize)) {
            std::cerr << "ZBON: EncodedData object has size value of " << encodedData.size
                      << "(" << encodedData.numHandles << " handles), but is actually "
                      << position << "(" << (handlePosition/HANDLE_SIZE) << " handles) bytes big."
                      << std::endl;
            std::cerr << "size: " << encodedData.size << std::endl
                      << "position: " << position << std::endl
                      << "numHandles:" << encodedData.numHandles << std::endl
                      << "handlePosition:" << handlePosition << std::endl
                      << "handlesSize: " << handlesSize << std::endl
                      << "A: " << (position != encodedData.size) << std::endl
                      << "B: " << (handlePosition != handlesSize) << std::endl;
            throw DecoderException();
        }
    }
};

template<typename T>
static EncodedData encode(const T &cppData) {
    Encoder e;
    return e.encode(cppData);
}
template<typename ...Types>
static EncodedData encodeObject(const Types &...cppData) {
    Encoder e;
    return e.encodeObject(cppData...);
}


template<typename T>
static void decode(const EncodedData &encodedData, T &result) {
    Decoder d(encodedData);
    d.decode(result);
}

#define ZBON_ENCODING_FUNCTIONS(...) \
    static constexpr zbon::Type ZBONType() { \
        return zbon::Type::OBJECT; \
    } \
    zbon::Size ZBONSize() const { \
        return zbon::sizeForObject(__VA_ARGS__); \
    } \
    void ZBONEncode(zbon::Encoder &encoder) const { \
        encoder.encodeObjectValue(__VA_ARGS__); \
    } \
    void ZBONDecode(zbon::Decoder &decoder) { \
        decoder.decodeFromObject(__VA_ARGS__); \
    }

}
