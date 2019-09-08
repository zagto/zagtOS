#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/syscall.h>

using namespace zagtos;

struct SpawnProcessArgs {
    size_t binarySize;
    const unsigned char *binaryData;

    const Protocol *canUse;
    size_t numCanUse;
    const Protocol *askUse;
    size_t numAskUse;
    const Protocol *canProvide;
    size_t numCanProvide;

    MessageType messageType;
    size_t messageSize;
    const unsigned char *messageData;

    uuid_t result;
};

void zagtos::environmentSpawn(const ExternalBinary &binary,
                              std::vector<Protocol> canUseProtocols,
                              std::vector<Protocol> askUseProtocols,
                              std::vector<Protocol> canProvideProtocols,
                              const MessageType &messageType,
                              zbon::EncodedData runMessage) {
    SpawnProcessArgs args {
        binary.size(),
        binary.data(),
        canUseProtocols.data(),
        canUseProtocols.size(),
        askUseProtocols.data(),
        askUseProtocols.size(),
        canProvideProtocols.data(),
        canProvideProtocols.size(),
        messageType,
        runMessage.size(),
        runMessage.data(),
        {0}
    };

    zagtos_syscall1(SYS_SPAWN_PROCESS, reinterpret_cast<size_t>(&args));
}
