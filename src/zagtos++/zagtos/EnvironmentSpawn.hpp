#ifndef ENVIRONMENTSPAWN_HPP
#define ENVIRONMENTSPAWN_HPP

#include <vector>
#include <zagtos/zbon.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/ExternalBinary.hpp>

namespace zagtos {
    enum Priority {
        IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_FOREGROUND
    };

    void environmentSpawn(const ExternalBinary &binary,
                          Priority priority,
                          std::vector<uint32_t> canUseTags,
                          const MessageType &messageType,
                          zbon::EncodedData runMessage);
}

#endif // ENVIRONMENTSPAWN_HPP
