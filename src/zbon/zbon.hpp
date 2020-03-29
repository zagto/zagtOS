#ifndef ZBON_HPP
#define ZBON_HPP

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <vector>
#include <array>
#include <tuple>
#include <limits>
#include <iostream>

namespace zbon {
    enum class Type : uint8_t {
        OBJECT, NOTHING, ARRAY, STRING, BOOLEAN,
        INT8, UINT8, INT16, UINT16, INT32, UINT32, INT64, UINT64,
        FLOAT, DOUBLE, HANDLE
    };
    static const size_t NUM_TYPES = 16;
    static const size_t HANDLE_SIZE = 4;

    struct Size {
        size_t numRegularBytes{0};
        size_t numHandles{0};
    };

    static Size operator+(const Size &a, const Size &b) {
        return {a.numRegularBytes + b.numRegularBytes, a.numHandles + b.numHandles};
    }

    static std::ostream &operator<<(std::ostream &stream, Type type) {
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
        assert(false);
    }

    /* typeFor is a class and not a function to allow partial specialization */
    template<typename T> class typeFor {
    public:
        static constexpr Type type() {
            return T::ZBONType();
        }
    };
    template<> class typeFor<bool> {
    public:
        static constexpr Type type() {
            return Type::BOOLEAN;
        }
    };
    template<> class typeFor<uint8_t> {
    public:
        static constexpr Type type() {
            return Type::UINT8;
        }
    };
    template<> class typeFor<const uint8_t> {
    public:
        static constexpr Type type() {
            return Type::UINT8;
        }
    };
    template<> class typeFor<int8_t> {
    public:
        static constexpr Type type() {
            return Type::INT8;
        }
    };
    template<> class typeFor<uint16_t> {
    public:
        static constexpr Type type() {
            return Type::UINT16;
        }
    };
    template<> class typeFor<int16_t> {
    public:
        static constexpr Type type() {
            return Type::INT16;
        }
    };
    template<> class typeFor<uint32_t> {
    public:
        static constexpr Type type() {
            return Type::UINT32;
        }
    };
    template<> class typeFor<int32_t> {
    public:
        static constexpr Type type() {
            return Type::INT32;
        }
    };
    template<> class typeFor<uint64_t> {
    public:
        static constexpr Type type() {
            return Type::UINT64;
        }
    };
    template<> class typeFor<int64_t> {
    public:
        static constexpr Type type() {
            return Type::INT64;
        }
    };
    template<> class typeFor<float> {
    public:
        static constexpr Type type() {
            return Type::FLOAT;
        }
    };
    template<> class typeFor<double> {
    public:
        static constexpr Type type() {
            return Type::DOUBLE;
        }
    };
    template<> class typeFor<std::string> {
    public:
        static constexpr Type type() {
            return Type::STRING;
        }
    };
    template<typename T, std::size_t count> class typeFor<const std::array<T, count>> {
    public:
        static constexpr Type type() {
            return Type::ARRAY;
        }
    };
    template<typename T, std::size_t count> class typeFor<std::array<T, count>> {
    public:
        static constexpr Type type() {
            return Type::ARRAY;
        }
    };
    template<typename T> class typeFor<const std::vector<T>> {
    public:
        static constexpr Type type() {
            return Type::ARRAY;
        }
    };
    template<typename T> class typeFor<std::vector<T>> {
    public:
        static constexpr Type type() {
            return Type::ARRAY;
        }
    };
    template<typename T, size_t count> class typeFor<const T[count]> {
    public:
        static constexpr Type type() {
            return Type::ARRAY;
        }
    };
    template<typename T, size_t count> class typeFor<T[count]> {
    public:
        static constexpr Type type() {
            return Type::ARRAY;
        }
    };
    template<typename ...Types> class typeFor<std::tuple<Types...>> {
    public:
        static constexpr Type type() {
            return Type::OBJECT;
        }
    };

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
    static const size_t HEADER_SIZE = TYPE_SIZE + COUNT_SIZE;

    extern "C" void *memcpy(void *dest, const void *src, size_t len);

    static void copyConvertEndianness(void *destination, const void *source, size_t length) {
        if constexpr (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
            memcpy(destination, source, length);
        } else {
            for (size_t i = 0; i < length; i++) {
                static_cast<uint8_t *>(destination)[i] =
                        static_cast<const uint8_t *>(source)[length - i - 1];
            }
        }
    }

    static Size sizeFor(bool) {
        return {1};
    }
    template<typename T>
    static Size sizeFor(const T& object) {
        return object.ZBONSize();
    }
    static Size sizeFor(std::string string) {
        return {TYPE_SIZE + COUNT_SIZE + string.size()};
    }
#define NUMBER_TYPE(T) static Size sizeFor(const T) { return {sizeof(T)}; }
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
    static Size sizeFor(const std::vector<T> &vector) {
        Size sum = {COUNT_SIZE * 2 + TYPE_SIZE};
        for (const T &element: vector) {
            sum += sizeFor(element);
        }
        return sum;
    }
    template<typename T, size_t count>
    static Size sizeFor(const std::array<T, count> &array) {
        Size sum = {COUNT_SIZE * 2 + TYPE_SIZE};
        for (const T &element: array) {
            sum += sizeFor(element);
        }
        return sum;
    }
    template<typename T, size_t count>
    static Size sizeFor(const T (&array)[count]) {
        Size sum = {COUNT_SIZE * 2 + TYPE_SIZE};
        for (const T &element: array) {
            sum += sizeFor(element);
        }
        return sum;
    }
    template<size_t position, typename ...Types>
    static Size sizeForTupleElements(const std::tuple<Types...> &tuple) {
        if constexpr(position == std::tuple_size<std::tuple<Types...>>::value) {
            return {};
        } else {
            const auto &element = std::get<position>(tuple);
            return Size{TYPE_SIZE} + sizeFor(element) + sizeForTupleElements<position + 1>(tuple);
        }
    }
    template<typename ...Types>
    static Size sizeFor(const std::tuple<Types...> &tuple) {
        return sizeForTupleElements<0, Types...>(tuple) + Size{COUNT_SIZE * 2};
    }


    /*class Schema {
    private:
        Type type;

    protected:
        Schema(Type type) :
            type{type} {}
    };

    class ArraySchema : public Schema {
        Schema &elements;
    };

    template <size_t numOptions>
    class OptionSchema : public Schema {
        std::array<Schema &, numOptions> options;
    };

    template <size_t numProperties>
    class ObjectSchema : public Schema {
        std::array<Schema &, numProperties> properties;
    };

    class NothingSchema : public Schema {
    public:
        NothingSchema() :
            Schema(Type::NOTHING) {}
    };

    template <typename T>
    class NumberSchema : public Schema {
        T minimum;
        T maximum;

    public:
        NumberSchema(T minimum = std::numeric_limits<T>::min(),
                     T maximum = std::numeric_limits<T>::max()) :
            Schema(typeFor<T>()),
            minimum{minimum},
            maximum{maximum} {}
    };*/

    class EncodedData {
    protected:
        friend class Encoder;
        friend class Decoder;
        uint8_t *_data;
        size_t _size;
        size_t _numHandles;

        EncodedData(uint8_t *data, size_t size):
            _data{data},
            _size{size} {}

    public:
        EncodedData():
            _data{nullptr},
            _size{0},
            _numHandles{0} {}
        EncodedData(EncodedData &other) = delete;
        EncodedData(EncodedData &&other):
                _data{other._data},
                _size{other._size},
                _numHandles{other._numHandles} {
            other._data = nullptr;
            other._size = 0;
            other._numHandles = 0;
        }
        ~EncodedData() {
            if (_data != nullptr) {
                delete[] _data;
            }
        }
        uint8_t *data() {
            return _data;
        }
        size_t size() {
            return _size;
        }
        size_t numHandles() {
            return _numHandles;
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

        /* The bytesSize field is filled out when the end of the element is reached so the value
         * is known. Other than that it's just a 64-bit unsigned number */
        void encodeBytesSize(size_t bytesSize, size_t bytesSizePosition) {
            encodeNumber(static_cast<uint64_t>(bytesSize), bytesSizePosition);
        }

        template<typename T>
        void encodeArray(Type elementType, const T &array, size_t length) {
            encodeType(elementType);
            encodeValue(static_cast<uint64_t>(length));
            /* insert bytesSize (64bit) later */
            size_t bytesSizePosition = position;
            position += COUNT_SIZE;

            for (const auto &element: array) {
                encodeValue(element);
            }
            size_t bytesSize = position - bytesSizePosition - COUNT_SIZE;
            encodeBytesSize(bytesSize, bytesSizePosition);
        }

        template<typename T>
        void encodeNumber(T value, size_t &numberPosition) {
            copyConvertEndianness(&data[numberPosition], &value, sizeof(T));
            numberPosition += sizeof(T);
        }
        template<size_t position, typename Head, typename ...Tail>
        Type nthType() {
            if constexpr(position == 0) {
                return typeFor<Head>::type();
            } else {
                return nthType<position - 1, Tail...>();
            }
        }

        template<size_t position, typename ...Types>
        void encodeTupleElements(const std::tuple<Types...> &tuple) {
            if constexpr(position == std::tuple_size<std::tuple<Types...>>::value) {
                return;
            } else {
                const auto &element = std::get<position>(tuple);
                encodeType(nthType<position, Types...>());
                encodeValue(element);
                encodeTupleElements<position + 1, Types...>(tuple);
            }
        }
    public:
        void encodeValue(bool value) {
            data[position] = value;
            position += 1;
        }
#define NUMBER_TYPE(T) void encodeValue(T value) { encodeNumber(value, position); }
        INSERT_NUMBER_TYPES
#undef NUMBER_TYPE
        template<typename T>
        void encodeValue(const std::vector<T> &vector) {
            encodeArray(vector, vector.size());
        }
        template<typename T, size_t count>
        void encodeValue(const std::array<T, count> &array) {
            encodeArray(typeFor<T>::type(), array, count);
        }
        template<typename T, size_t count>
        void encodeValue(const T (&array)[count]) {
            encodeArray(typeFor<T>::type(), array, count);
        }
        void encodeValue(std::string string) {
            encodeValue(static_cast<uint64_t>(string.size()));
            for (size_t i = 0; i < string.size(); i++) {
                encodeValue(string[i]);
            }
        }
        template<typename ...Types>
        void encodeValue(const std::tuple<Types...> &tuple) {
            encodeValue(static_cast<uint64_t>(sizeof(tuple)));
            /* insert bytesSize (64bit) later */
            size_t bytesSizePosition = position;
            position += COUNT_SIZE;

            encodeTupleElements<0, Types...>(tuple);

            size_t bytesSize = position - bytesSizePosition - COUNT_SIZE;
            encodeBytesSize(bytesSize, bytesSizePosition);
        }
        void encodeHandle(const uint32_t handle) {
            encodeNumber(handle, handlePosition);
        }
        template<typename T>
        void encodeValue(const T& object) {
            object.ZBONEncode(*this);
        }

    public:
        Encoder():
            data{nullptr},
            position{0},
            handlePosition{0} {}

        template<typename T>
        void encodeObjectProperty(const T &value) {
            assert(data != nullptr);

            encodeType(typeFor<T>::type());
            encodeValue(value);
        }

        /* when objects are encoded a type byte is put in front of every property */
        template<typename T>
        size_t sizeInObjectFor(const T &element) {
            return TYPE_SIZE + sizeFor(element);
        }

        template<typename T>
        EncodedData encode(const T &cppData) {
            /* the root element is encoded like an object property (with type byte), so the type of
             * the whole thing is known on decoding */
            Size size = sizeFor(cppData);
            size_t handlesSize = size.numHandles * HANDLE_SIZE;
            size_t bytesSize = size.numRegularBytes + handlesSize;

            data = new uint8_t[HEADER_SIZE + bytesSize];
            EncodedData encodedData(data, HEADER_SIZE + bytesSize);
            assert(position == 0);
            assert(handlePosition == 0);
            position = handlesSize;

            encodeType(typeFor<T>::type());
            encodeValue(static_cast<uint64_t>(bytesSize));
            encodeValue(cppData);

            /* check encoding actually used the amount of space the size calculation got */
            assert(position == HEADER_SIZE + bytesSize);
            assert(handlePosition == handlesSize);
            return encodedData;
        }
    };

    class Decoder {
    private:
        const EncodedData &encodedData;
        size_t position;
        size_t handlePosition;
        Type encodedType;

        bool decodeType(Type &result) {
            if (!(position < encodedData._size)) {
                std::cerr << "ZBON: Unexpected End of Data at " << encodedData._size << std::endl;
                return false;
            }

            uint8_t value = encodedData._data[position];
            if (value < NUM_TYPES) {
                result = static_cast<Type>(value);
                position++;
                return true;
            } else {
                std::cerr << "ZBON: Invalid Type Byte at " << position << std::endl;
                return false;
            }
        }

        template<typename T>
        bool verifyElement(Type elementType) {
            if (elementType != encodedType) {
                std::cerr << "ZBON: program expects a " << elementType
                          << " but ZBON data holds " << encodedType << std::endl;
                return false;
            }
        }

    public:
        template<typename T>
        bool decodeValue(T &result, Type encodedType) {
            if (typeFor<T>::type() == encodedType) {
                return decodeValue(result);
            } else {
                std::cerr << "ZBON: requested decode of " << typeFor<T>::type()
                          << " but ZBON data holds " << encodedType << std::endl;
                return false;
            }
        }

        bool decodeValue(bool &result) {
            if (!(position < encodedData._size)) {
                std::cerr << "ZBON: Unexpected End of Data at " << encodedData._size << std::endl;
                return false;
            }

            result = static_cast<bool>(encodedData._data[position]);
            position++;
            return true;
        }

        template<typename T>
        bool decodeNumber(T &result, size_t &position) {
            if (!(position < encodedData._size && encodedData._size - position >= sizeof(T))) {
                std::cerr << "ZBON: Unexpected End of Data at " << encodedData._size << std::endl;
                return false;
            }

            copyConvertEndianness(&result, &encodedData._data[position], sizeof(T));
            position += sizeof(T);
            return true;
        }
#define NUMBER_TYPE(T) bool decodeValue(T &result) { return decodeNumber(result, position); }
        INSERT_NUMBER_TYPES
#undef NUMBER_TYPE

        template<typename ArrayType, typename ElementType>
        bool decodeArray(ArrayType &resultArray) {
            Type type;
            if (!decodeType(type)) {
                return false;
            }
            if (typeFor<ElementType>::type() != type) {
                std::cerr << "ZBON: requested decode of Array of " << typeFor<ElementType>::type()
                          << " but ZBON Array holds " << type << std::endl;
                return false;
            }

            uint64_t numElements;
            if (!decodeValue(numElements)) {
                return false;
            }
            if (numElements != std::size(resultArray)) {
                std::cerr << "ZBON: requested decode of Array of " << std::size(resultArray)
                          << " elements but ZBON Array holds " << numElements << " elements."
                          << std::endl;
                return false;
            }

            uint64_t bytesSize;
            if (!decodeValue(bytesSize)) {
                return false;
            }
            size_t positionBeforeData = position;

            for (auto &element: resultArray) {
                if (!decodeValue(element, type)) {
                    return false;
                }
            }

            if (position - positionBeforeData != bytesSize) {
                std::cerr << "ZBON: Array array has bytesSize value of " << bytesSize
                          << " but is actually " << (positionBeforeData - position) << " bytes big."
                          << std::endl;
                return false;
            }
            return true;
        }

        bool decodeHandle(uint32_t &result) {
            return decodeNumber(result, handlePosition);
        }

        template<typename T, std::size_t length>
        bool decodeValue(std::array<T, length> &result) {
            return decodeArray<std::array<T, length>, T>(result);
        }
        template<typename T, size_t length>
        bool decodeValue(T (&result)[length]) {
            return decodeArray<T[length], T>(result);
        }

        template<typename T>
        bool decodeValue(std::vector<T> &result) {
            Type type;
            if (!decodeType(type)) {
                return false;
            }
            if (typeFor<T>::type() != type) {
                std::cerr << "ZBON: requested decode of Array of " << typeFor<T>::type()
                          << " but ZBON Array holds " << type << std::endl;
                return false;
            }

            uint64_t numElements;
            if (!decodeValue(numElements)) {
                return false;
            }
            result.resize(numElements);

            uint64_t bytesSize;
            if (!decodeValue(bytesSize)) {
                return false;
            }
            size_t positionBeforeData = position;

            for (auto &element: result) {
                if (!decodeValue(element, type)) {
                    return false;
                }
            }

            if (position - positionBeforeData != bytesSize) {
                std::cerr << "ZBON: Array array has bytesSize value of " << bytesSize
                          << " but is actually " << (positionBeforeData - position) << " bytes big."
                          << std::endl;
                return false;
            }
            return true;
        }

        bool decodeValue(std::string &result) {
            uint64_t numElements;
            if (!decodeValue(numElements)) {
                return false;
            }
            result.resize(numElements);

            for (auto &element: result) {
                if (!decodeValue(element)) {
                    return false;
                }
            }
            return true;
        }

        template<typename T>
        bool decodeValue(T& object) {
            object.ZBONDecode(*this);
        }

        Decoder(const EncodedData &encodedData) :
            encodedData{encodedData},
            position{encodedData._numHandles * HANDLE_SIZE},
            handlePosition{0} {}

        template<typename T>
        bool decode(T &result) {
            Type rootType;
            if (!decodeType(rootType)) {
                return false;
            }

            uint64_t handlesSize = encodedData._numHandles * HANDLE_SIZE;
            uint64_t regularBytesSize;
            if (!decodeValue(regularBytesSize)) {
                return false;
            }
            if (handlesSize + regularBytesSize + HEADER_SIZE != encodedData._size) {
                std::cerr << "ZBON: got " << encodedData._size << " bytes of data but encoded data "
                          << "is only " << (regularBytesSize + HEADER_SIZE) << " bytes." << std::endl;
                return false;
            }

            assert(position == HEADER_SIZE + handlesSize);
            if (!decodeValue(result, rootType)) {
                return false;
            }

            if (position != HEADER_SIZE + regularBytesSize + handlesSize
                    || handlePosition != handlesSize) {
                std::cerr << "ZBON: root element has bytesSize value of " << regularBytesSize
                          << "but is actually " << (position - HEADER_SIZE) << " bytes big."
                          << std::endl;
                return false;
            }
            return true;
        }
    };

    template<typename T>
    static EncodedData encode(const T &cppData) {
        Encoder e;
        return e.encode(cppData);
    }

    template<typename T>
    static bool decode(const EncodedData &encodedData, T &result) {
        Decoder d(encodedData);
        return d.decode(result);
    }

}

#endif // ZBON_HPP
