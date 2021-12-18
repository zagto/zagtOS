#pragma once

#include <cstdint>
#include <zagtos/ZBON.hpp>

namespace zagtos {
    class HandleObject {
    protected:
        static constexpr uint32_t INVALID_HANDLE{static_cast<uint32_t>(-1)};

        uint32_t _handle{INVALID_HANDLE};

        HandleObject(const uint32_t handle);

    public:
        HandleObject();
        HandleObject(HandleObject &) = delete;
        ~HandleObject();
        HandleObject(HandleObject &&other);
        HandleObject &operator=(HandleObject &&other);

        static constexpr zbon::Type ZBONType() {
            return zbon::Type::HANDLE;
        }
        zbon::Size ZBONSize() const;
        void ZBONEncode(zbon::Encoder &encoder) const;
        void ZBONDecode(zbon::Decoder &decoder);
    };
}
