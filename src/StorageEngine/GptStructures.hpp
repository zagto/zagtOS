#pragma once

#include <cstdint>
#include <cstddef>

// TODO: don't assume little-endian

struct MixedEndianUUID {
    uint64_t low, high; // TODO
};

struct GptHeader {
    char signature[8];
    uint32_t revision;
    uint32_t headerSize;
    uint32_t headerChceksum;
    uint64_t currentLba;
    uint64_t backupLba;
    uint64_t firstUsableLba;
    uint64_t lastUsableLba;
    MixedEndianUUID diskID;
    uint64_t firstPartitionEntryLba;
    uint32_t numPartitionEntries;
    uint32_t partitionEntriesSize;
    uint32_t partitionEntriesChecksum;
};

struct PartitionEntry {
    MixedEndianUUID partitionType;
    MixedEndianUUID id;
    uint64_t firstLba;
    uint64_t lastLba;
    uint64_t attributeFlags;
    uint16_t paritionName[72/2];
};
