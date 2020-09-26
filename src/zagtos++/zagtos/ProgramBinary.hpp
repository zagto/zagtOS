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
    size_t entryAddress;
    std::optional<zagtos::ProgramSection> TLSSection;
    std::vector<zagtos::ProgramSection> sections;

    ZBON_ENCODING_FUNCTIONS(entryAddress, TLSSection, sections);
};

}
