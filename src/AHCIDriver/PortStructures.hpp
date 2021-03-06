#pragma once

#define _GNU_SOURCE 1
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits.h>

enum FISType {
    REG_H2D = 0x27,
    REG_D2H = 0x34,
    DMA_ACT = 0x39,
    DMA_SETUP = 0x41,
    DATA = 0x46,
    BIST = 0x58,
};

enum class ATACommand {
    IDENTIFY_DEVICE = 0xec,
};

class H2DFIS {
private:
    static const uint8_t COMMAND_BIT{1u<<7};

    uint8_t type;
    uint8_t commandBitPMPort;
    uint8_t _command;
    uint8_t featuresLow;
    uint32_t deviceLBALow;
    uint32_t featuresLBAHigh;
    uint32_t controlICCCount;
    uint32_t reserved;
    uint32_t crc;

public:
    H2DFIS() {
        memset(this, 0, sizeof(H2DFIS));
        type = FISType::REG_H2D;
    }
    void command(ATACommand value) {
        _command = static_cast<uint8_t>(value);
        commandBitPMPort |= COMMAND_BIT;
    }
};

class DSFISClass {
    int todo;
};

class PSFISClass {
    int todo;

};


class RFISClass {
    int todo;

};

class UFISClass {
    int todo;

};

struct PhysicalRegionDescriptor {
    uint32_t DBA;
    uint32_t DBAU;
    uint32_t reserved0;
    uint32_t DBC:22, reserved1:9, I:1;
};

struct CommandTable {
    static const size_t PRDT_OFFSET = 0x80;
    static const size_t PRDT_MAX_ENTRIES = (PAGE_SIZE - PRDT_OFFSET) / sizeof(PhysicalRegionDescriptor);

    union {
        H2DFIS h2d;
    };
    uint8_t reserved[PRDT_OFFSET - sizeof(H2DFIS)];
    std::array<PhysicalRegionDescriptor, PRDT_MAX_ENTRIES> PRDT;
};


struct CommandHeader {
    uint32_t CFL:5, ATAPI:1, W:1, P:1, R:1, B:1, C:1, reserved0:1, PMP:4, PRDTL:16;
    uint32_t PRDBC;
    uint32_t CTBA0;
    uint32_t CTBA_U0;
    uint32_t reserved1[4];
};

static const size_t MAX_NUM_COMMAND_SLOTS = 32;
