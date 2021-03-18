#include <cstring>
#include <zagtos/EnvironmentSpawn.hpp>
#include <zagtos/ExternalBinary.hpp>
#include <zagtos/syscall.h>
#include <zagtos/ProgramBinary.hpp>

using namespace zagtos;

struct SpawnProcessArgs {
    size_t entryAddress;
    size_t numSections;
    void *sectionsAddress;
    void *TLSSection;
    size_t priority;

    UUID messageType;
    const unsigned char *messageAddress;
    size_t messageSize;
    uint32_t numMessageHandles;

    const char* logName;
    size_t logNameSize;
};

/* Points to the same data as a ProgramSection. Used to pass data to kernel */
struct SpawnProcessSection {
    size_t address;
    size_t sizeInMemory;
    size_t flags;
    size_t dataSize;
    const void *dataAddress;

    SpawnProcessSection(const ProgramSection &programSection) :
        address{programSection.address},
        sizeInMemory{programSection.sizeInMemory},
        flags{programSection.flags},
        dataSize{programSection.data.size()},
        dataAddress{programSection.data.data()} {}
};

void zagtos::environmentSpawn(const ExternalBinary &binary,
                              Priority priority,
                              const UUID messageType,
                              zbon::EncodedData runMessage) {
    ProgramBinary program;
    zbon::decode(binary.data(), program);

    std::vector<SpawnProcessSection> sections(program.sections.begin(), program.sections.end());
    std::optional<SpawnProcessSection> tls(program.TLSSection);

    SpawnProcessArgs args = {
        .entryAddress = program.entryAddress,
        .numSections = program.sections.size(),
        .sectionsAddress = sections.data(),
        .TLSSection = tls ? &*tls : nullptr,
        .priority = priority,
        .messageType = messageType, /* inserted below */
        .messageAddress = runMessage.data(),
        .messageSize = runMessage.size(),
        .numMessageHandles = static_cast<uint32_t>(runMessage.numHandles()),
        .logName = binary.logName(),
        .logNameSize = strlen(binary.logName()),
    };

    size_t ok = zagtos_syscall1(SYS_SPAWN_PROCESS, reinterpret_cast<size_t>(&args));
    if (ok == 0) {
        throw std::invalid_argument("invalid executable to spawn process");
    }
}
