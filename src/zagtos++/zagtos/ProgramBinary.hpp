#pragma once

#include <zagtos/ZBON.hpp>

namespace zagtos {

struct ProgramSection {
    uint64_t address;
    uint64_t sizeInMemory;
    uint64_t flags;
    std::vector<uint8_t> data;

    ZBON_ENCODING_FUNCTIONS(address, sizeInMemory, flags, data);
};

struct ProgramBinary {
    uint64_t entryAddress;
    std::vector<zagtos::ProgramSection> sections;

    ZBON_ENCODING_FUNCTIONS(entryAddress, sections);
};

}
