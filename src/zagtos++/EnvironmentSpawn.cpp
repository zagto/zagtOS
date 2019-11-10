#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/syscall.h>

using namespace zagtos;

struct SpawnProcessArgs {
    const unsigned char *binaryData;
    size_t binarySize;

    size_t priority;

    const uint32_t *canUse;
    size_t numCanUse;

    MessageType messageType;
    size_t messageSize;
    const unsigned char *messageAddress;

    uuid_t result;
};

void zagtos::environmentSpawn(const ExternalBinary &binary,
                              Priority priority,
                              std::vector<uint32_t> canUseTags,
                              const MessageType &messageType,
                              zbon::EncodedData runMessage) {
    SpawnProcessArgs args {
        binary.data(),
        binary.size(),
        static_cast<size_t>(priority),
        canUseTags.data(),
        canUseTags.size(),
        messageType,
        runMessage.size(),
        runMessage.data(),
        {0}
    };

    zagtos_syscall1(SYS_SPAWN_PROCESS, reinterpret_cast<size_t>(&args));
}
