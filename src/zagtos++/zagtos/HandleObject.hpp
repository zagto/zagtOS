#pragma once

#include <cstdint>
#include <zagtos/ZBON.hpp>
#include <zagtos/KernelApi.h>

namespace zagtos {
    class HandleObject {
    protected:
        /* port and Interrupt need to pass an EventQueue handle */
        friend class Interrupt;
        friend class Port;
        uint32_t _handle{cApi::ZAGTOS_INVALID_HANDLE};

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
