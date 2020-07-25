#include <cstring>
#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/syscall.h>

using namespace zagtos;

struct SpawnProcessArgs {
    const unsigned char *binaryData;
    size_t binarySize;

    size_t priority;

    uuid_t messageType;
    const unsigned char *messageAddress;
    size_t messageSize;
    uint32_t numMessageHandles;

    const char* logName;
    size_t logNameSize;

    uint32_t result;
};

void zagtos::environmentSpawn(const ExternalBinary &binary,
                              Priority priority,
                              const uuid_t messageType,
                              zbon::EncodedData runMessage) {
    SpawnProcessArgs args {
        binary.data(),
        binary.size(),
        static_cast<size_t>(priority),
        {0},
        runMessage.data(),
        runMessage.size(),
        static_cast<uint32_t>(runMessage.numHandles()),
        binary.logName(),
        strlen(binary.logName()),
        0
    };

    uuid_copy(args.messageType, messageType);

    zagtos_syscall1(SYS_SPAWN_PROCESS, reinterpret_cast<size_t>(&args));
}
