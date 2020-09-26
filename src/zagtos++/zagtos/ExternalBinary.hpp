#pragma once

#include <stddef.h>
#include <zagtos/ZBON.hpp>

namespace zagtos {
    class ExternalBinary {
    protected:
        zbon::EncodedData _data;
        const char *_logName;

    public:
        constexpr ExternalBinary(const unsigned char *start,
                                 const unsigned char *end,
                                 const char *logName):
            _data(const_cast<unsigned char *>(start),
                  reinterpret_cast<size_t>(end) - reinterpret_cast<size_t>(start),
                  0,
                  true),
            _logName{logName} {}

        const zbon::EncodedData &data() const {
            return _data;
        }
        const char *logName() const {
            return _logName;
        }
    };
}

#define EXTERNAL_BINARY(name) \
extern const unsigned char _binary_ ## name ## _zbon_start; \
extern const unsigned char _binary_ ## name ## _zbon_end; \
static const zagtos::ExternalBinary name(&_binary_ ## name ## _zbon_start, \
                                         &_binary_ ## name ## _zbon_end, \
                                         #name);
