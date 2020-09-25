#include <exception>
#include <cstring>
#include <zagtos/ZBON.hpp>


namespace zbon {

std::ostream &operator<<(std::ostream &stream, Type type) {
    switch (type) {
    case Type::BINARY:
        return stream << "Binary";
    case Type::NOTHING:
        return stream << "Nothing";
    case Type::OBJECT:
        return stream << "Object";
    case Type::STRING:
        return stream << "String";
    case Type::BOOLEAN:
        return stream << "Boolean";
    case Type::INT8:
        return stream << "8-bit Signed Integer";
    case Type::UINT8:
        return stream << "8-bit Unsigned Integer";
    case Type::INT16:
        return stream << "16-bit Signed Integer";
    case Type::UINT16:
        return stream << "16-bit Unsigned Integer";
    case Type::INT32:
        return stream << "32-bit Signed Integer";
    case Type::UINT32:
        return stream << "32-bit Unsigned Integer";
    case Type::INT64:
        return stream << "64-bit Signed Integer";
    case Type::UINT64:
        return stream << "64-bit Unsigned Integer";
    case Type::FLOAT:
        return stream << "Single-Precision Float";
    case Type::DOUBLE:
        return stream << "Single-Precision Float";
    case Type::HANDLE:
        return stream << "Handle";
    default:
        throw std::logic_error("Tried to output non-existing ZBON type");
    }
}

void copyConvertEndianness(void *destination, const void *source, size_t length) {
    if constexpr (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
        memcpy(destination, source, length);
    } else {
        for (size_t i = 0; i < length; i++) {
            static_cast<uint8_t *>(destination)[i] =
                    static_cast<const uint8_t *>(source)[length - i - 1];
        }
    }
}

Size sizeFor(std::string string) {
    return {TYPE_SIZE + COUNT_SIZE + string.size()};
}

void Encoder::encodeValue(const zbon::EncodedData &value) {
    value.ensureVerified();
    size_t handleSize = value.numHandles() * HANDLE_SIZE;
    size_t numRegularBytes = value._size - handleSize;
    memcpy(data + position, value._data + handleSize, numRegularBytes);
    memcpy(data + handlePosition, value._data, handleSize);
    position += numRegularBytes;
    handlePosition += handleSize;
}

void Encoder::encodeBinary(const uint8_t *value, size_t length) {
    encodeType(Type::BINARY);
    encodeNumber(length, position);
    memcpy(data + position, value, length);
    position += length;
}

void Decoder::decodeValue(zbon::EncodedData &value) {
    assert(value._data == nullptr && value._size == 0);

    size_t headerPosition = position;

    Type type;
    decodeType(type);

    size_t length{0}, numHandles{0};

    switch (type) {
    case Type::BINARY:
    case Type::STRING:
        decodeNumber(length, position);
        break;
    case Type::NOTHING:
        /* do nothing */
        break;
    case Type::OBJECT:
        /* ignore number of elements of the object */
        position += COUNT_SIZE;

        decodeNumber(numHandles, position);
        decodeNumber(length, position);
        break;
    case Type::BOOLEAN:
    case Type::INT8:
    case Type::UINT8:
        length = 1;
        break;
    case Type::INT16:
    case Type::UINT16:
        length = 2;
        break;
    case Type::INT32:
    case Type::UINT32:
    case Type::FLOAT:
        length = 4;
        break;
    case Type::INT64:
    case Type::UINT64:
    case Type::DOUBLE:
        length = 8;
        break;
    case Type::HANDLE:
        numHandles = 1;
        break;
    default:
        throw std::logic_error("missing type case");
    }

    size_t dataLength = position - headerPosition + length;
    if (dataLength < length) {
        std::cerr << "ZBON: encoded length would cause overflow" << std::endl;
        throw DecoderException();
    }

    ensureEnoughLeft(dataLength);
    ensureEnoughHandlesLeft(numHandles);

    value._data = new uint8_t[numHandles * HANDLE_SIZE + dataLength];

    memcpy(value._data, encodedData._data + handlePosition, numHandles * HANDLE_SIZE);
    memcpy(value._data + numHandles * HANDLE_SIZE, encodedData._data + headerPosition, dataLength);

    position += length;
    handlePosition += numHandles * HANDLE_SIZE;
}

void Decoder::decodeBinary(uint8_t *buffer, size_t length) {
    decodeVerifyType(Type::BINARY);
    uint64_t encodedLength;
    decodeNumber(encodedLength, position);
    if (encodedLength != length) {
        std::cerr << "ZBON: decoding binary data of size " << length
                  << ", but got encoded data of size " << encodedLength << std::endl;
        throw DecoderException();
    }

    ensureEnoughLeft(length);
    memcpy(buffer, encodedData._data + position, length);
    position += length;
}


}
