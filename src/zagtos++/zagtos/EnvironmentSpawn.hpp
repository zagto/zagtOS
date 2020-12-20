#pragma once

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
                          const UUID messageType,
                          zbon::EncodedData runMessage);
}

