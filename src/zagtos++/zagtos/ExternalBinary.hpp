#ifndef EXTERNALBINARY_HPP
#define EXTERNALBINARY_HPP

#include <stddef.h>

namespace zagtos {
    class ExternalBinary {
    protected:
        const unsigned char *_data;
        size_t _size;

    public:
        constexpr ExternalBinary(const unsigned char *start, const unsigned char *end):
            _data{start},
            _size{reinterpret_cast<size_t>(end) - reinterpret_cast<size_t>(start)} {}

        const unsigned char *data() const {
            return _data;
        }
        size_t size() const {
            return _size;
        }
    };
}

#define EXTERNAL_BINARY(name) \
extern const unsigned char _binary_ ## name ## _start; \
extern const unsigned char _binary_ ## name ## _end; \
static const zagtos::ExternalBinary name(&_binary_ ## name ## _start, &_binary_ ## name ## _end);


#endif // EXTERNALBINARY_HPP
