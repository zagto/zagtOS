#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <optional>
#include <memory>
#include <elf.h>
#include <zagtos/ZBON.hpp>


static const unsigned char ExpectedELFIdentifier[9] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
                                                        ELFCLASS64,
                                                        ELFDATA2LSB,
                                                        1, /* ELF version 1? */
                                                        ELFOSABI_SYSV,
                                                        0};/* ABI version */

static Elf64_Ehdr FileHeader;

struct Section {
    uint64_t address;
    uint64_t sizeInMemory;
    std::vector<uint8_t> data;

    static constexpr zbon::Type ZBONType() {
        return zbon::Type::OBJECT;
    }
    zbon::Size ZBONSize() const {
        return zbon::sizeForObject(address, sizeInMemory, data);
    }
    void ZBONEncode(zbon::Encoder &encoder) const {
        encoder.encodeObjectValue(address, sizeInMemory, data);
    }
    void ZBONDecode(zbon::Decoder &decoder) {
        decoder.decodeFromObject(address, sizeInMemory, data);
    }
};

int main(int argc, char **argv) {
    if (argc != 3) {
        throw std::runtime_error("usage: CovertELF <input> <output> (but got "
                                 + std::to_string(argc)
                                 + " arguments instead)");
    }

    std::ifstream inFile(argv[1]);
    if (!inFile.is_open()) {
        throw std::runtime_error("could not open input file");
    }

    inFile.read(reinterpret_cast<char *>(&FileHeader), sizeof(Elf64_Ehdr));
    if (memcmp(FileHeader.e_ident, ExpectedELFIdentifier, sizeof(ExpectedELFIdentifier)) != 0) {
        throw std::runtime_error("input file is not a 64-bit litte endian System V ELF file");
    }
    if (FileHeader.e_type != ET_EXEC) {
        throw std::runtime_error("input file is an ELF file, but not of executable type");
    }

    if (FileHeader.e_phentsize != sizeof(Elf64_Phdr)) {
        throw std::runtime_error("input file's program headers don't have the expected size");
    }

    size_t numSections = FileHeader.e_phnum;
    std::vector<Elf64_Phdr> programHeaders(numSections);
    inFile.seekg(FileHeader.e_phoff, std::ios::beg);
    inFile.read(reinterpret_cast<char *>(programHeaders.data()), sizeof(Elf64_Phdr) * numSections);

    std::vector<Section> sections;
    sections.reserve(numSections);

    std::optional<Section> TLSSection;

    for (const auto &header: programHeaders) {
        if (!(header.p_type == PT_LOAD || header.p_type == PT_TLS)) {
            continue;
        }

        Section section = {
            .address = header.p_vaddr,
            .sizeInMemory = header.p_memsz,
            .data = std::vector<uint8_t>(header.p_filesz)
        };
        inFile.seekg(header.p_offset);
        inFile.read(reinterpret_cast<char *>(section.data.data()), header.p_filesz);

        if (header.p_type == PT_TLS) {
            if (TLSSection) {
                throw std::runtime_error("input file has more than one TLS section");
            }
            TLSSection = std::move(section);
        } else {
            sections.push_back(std::move(section));
        }
    }
    inFile.close();

    uint64_t entryAddress = FileHeader.e_entry;
    zbon::EncodedData output = zbon::encode(std::make_tuple(sections, TLSSection, entryAddress));

    std::ofstream outFile(argv[2]);
    if (!outFile.is_open()) {
        throw std::runtime_error("could not open output file");
    }
    assert(output.numHandles() == 0);
    outFile.write(reinterpret_cast<char *>(output.data()), output.size());
    outFile.close();
}
