#ifndef ENVIRONMENTSPAWN_HPP
#define ENVIRONMENTSPAWN_HPP

#include <vector>
#include <zagtos/ZBON.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/ExternalBinary.hpp>

namespace zagtos {
    enum Priority {
        IDLE, BACKGROUND, FOREGROUND, INTERACTIVE_FOREGROUND
    };

    void environmentSpawn(const ExternalBinary &binary,
                          Priority priority,
                          const uuid_t messageType,
                          zbon::EncodedData runMessage);
}

#endif // ENVIRONMENTSPAWN_HPP
