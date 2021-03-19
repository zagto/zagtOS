#pragma once

#include <common/inttypes.hpp>
#include <memory>
#include <processes/UUID.hpp>

class Process;

struct SpawnProcessSection {
    static const size_t FLAG_EXECUTABLE{1};
    static const size_t FLAG_WRITEABLE{2};
    static const size_t FLAG_READABLE{4};

    size_t address;
    size_t sizeInMemory;
    size_t flags;
    size_t dataSize;
    size_t dataAddress;

    Permissions permissions() const;
    Region region() const;
};

class SpawnProcessStruct {
private:
    size_t entryAddress;
    size_t numSections;
    size_t sectionsAddress;
    size_t TLSSectionAddress;
    size_t priority;

    UUID messageType;
    size_t messageAddress;
    size_t messageSize;
    uint32_t numMessageHandles;

    size_t logNameAddress;
    size_t logNameSize;
public:

    Result<size_t> perform(const shared_ptr<Process> &process);
};

Result<size_t> SpawnProcess(const shared_ptr<Process> &process,
                            uint64_t structAddress,
                            uint64_t,
                            uint64_t,
                            uint64_t,
                            uint64_t);
