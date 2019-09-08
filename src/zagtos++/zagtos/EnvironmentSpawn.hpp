#ifndef ENVIRONMENTSPAWN_HPP
#define ENVIRONMENTSPAWN_HPP

#include <vector>
#include <zagtos/zbon.hpp>
#include <zagtos/Messaging.hpp>
#include <zagtos/ExternalBinary.hpp>

namespace zagtos {
    void environmentSpawn(const ExternalBinary &binary,
                          std::vector<Protocol> canUseProtocols,
                          std::vector<Protocol> askUseProtocols,
                          std::vector<Protocol> canProvideProtocols,
                          const MessageType &messageType,
                          zbon::EncodedData runMessage);
}

#endif // ENVIRONMENTSPAWN_HPP
