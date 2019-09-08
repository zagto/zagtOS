#ifndef HAL_HPP
#define HAL_HPP

#include <uuid/uuid.h>
//#include <zagtos/Messaging.hpp>

namespace zagtos {
    UUID_DEFINE(StartHALMessage,
                0x4b, 0x87, 0xe1, 0xfc, 0x7f, 0x7a, 0x4f, 0x1c,
                0x9f, 0x1a, 0xaf, 0x3a, 0x38, 0x6d, 0x79, 0x89);
    UUID_DEFINE(StartHALResponse,
                0xa8, 0x24, 0x82, 0xcf, 0x8c, 0xde, 0x43, 0x03,
                0xb4, 0x45, 0x3b, 0xf0, 0x56, 0x4a, 0xb6, 0xea);
    /*class StartHALMessage {
    public:
        Port responsePort;

        zbon::Type ZBONType() {
            return zbon::Type::OBJECT;
        }
        size_t ZBONSize() {

        }
        void ZBONEncode(zbon::Encoder &encoder) {
            encoder.encodeValue(std::make_tuple(StartHALResponseID, responsePort));
        }
    };

    class StartHALResponse {
    public:
        zbon::Type ZBONType() {
            return zbon::Type::OBJECT;
        }
        void ZBONEncode(zbon::Encoder &encoder) {
            encoder.encodeValue(std::make_tuple(StartHALResponseID));
        }
    };*/
}

#endif // HAL_HPP
