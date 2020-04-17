#include <exception>
#include <cstring>
#include <zagtos/ZBON.hpp>


namespace zagtos {
namespace zbon {

std::ostream &operator<<(std::ostream &stream, Type type) {
    switch (type) {
    case Type::OBJECT:
        return stream << "Object";
    case Type::NOTHING:
        return stream << "Nothing";
    case Type::ARRAY:
        return stream << "Array";
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
    }
    throw std::logic_error("Tried to output non-existing ZBON type");
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

void EncodedData::ZBONEncode(Encoder &encoder) const {
    encoder.encodeCArray(Type::UINT8, _data, _size);
}

bool EncodedData::ZBONDecode(Decoder &decoder) {
    assert(_data == nullptr);
    _numHandles = 0;
    return decoder.decodeCArray(_data, _size);
}


Size sizeFor(std::string string) {
    return {TYPE_SIZE + COUNT_SIZE + string.size()};
}


}
}
