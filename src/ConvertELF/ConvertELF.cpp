#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <optional>
#include <memory>
#include <filesystem>
#include <elf.h>
#include <zagtos/ZBON.hpp>
#include <zagtos/ProgramBinary.hpp>


static const unsigned char ExpectedELFIdentifier[9] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
                                                        ELFCLASS64,
                                                        ELFDATA2LSB,
                                                        1, /* ELF version 1? */
                                                        ELFOSABI_SYSV,
                                                        0};/* ABI version */

static Elf64_Ehdr FileHeader;

#ifdef ZAGTOS_ARCH_x86_64
static const size_t PAGE_SIZE = 0x1000;
#else
#error "Please add page size definition for arch"
#endif

int main(int argc, char **argv) {
    if (argc != 3) {
        throw std::runtime_error("usage: CovertELF <input> <output> (but got "
                                 + std::to_string(argc)
                                 + " arguments instead)");
    }
    /* TODO: don't use tmpnam */
    char *tmpFileName = tmpnam(nullptr);
    const char *stripProgram = getenv("STRIP");
    if (stripProgram == nullptr) {
        throw std::runtime_error("please define STRIP environment variable");
    }
    std::filesystem::copy(argv[1], tmpFileName);
    if (system((std::string(stripProgram) + " " + tmpFileName).c_str())) {
        throw std::runtime_error("could not strip input file");
    }

    std::ifstream inFile(tmpFileName);
    if (!inFile.is_open()) {
        throw std::runtime_error("could not open temporary file");
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

    std::vector<zagtos::ProgramSection> sections;
    sections.reserve(numSections);

    std::optional<zagtos::ProgramSection> TLSSection;

    for (const auto &header: programHeaders) {
        if (!(header.p_type == PT_LOAD || header.p_type == PT_TLS)) {
            continue;
        }

        size_t offsetBefore = header.p_vaddr % PAGE_SIZE;
        size_t offsetAfter = (PAGE_SIZE - (header.p_vaddr + header.p_filesz)
                              % PAGE_SIZE) % PAGE_SIZE;
        size_t offsetAfterInMemory = (PAGE_SIZE - (header.p_vaddr + header.p_memsz)
                                      % PAGE_SIZE) % PAGE_SIZE;

        std::cerr << "offsetBefore "<<offsetBefore<<", offsetAfter "<<offsetAfter<<", offsetAfterInMemory "<<offsetAfterInMemory<<std::endl;
        zagtos::ProgramSection section = {
            .address = header.p_vaddr - offsetBefore,
            .sizeInMemory = offsetBefore + header.p_memsz + offsetAfterInMemory,
            .flags = header.p_flags,
            .data = std::vector<uint8_t>(offsetBefore + header.p_filesz + offsetAfter, 0)
        };
        /* for TLS, the address is not where it should be loaded, but the master TLS address, which
         * may be unaligned */
        if (header.p_type == PT_TLS) {
            section.address = header.p_vaddr;
        }

        inFile.seekg(header.p_offset);
        inFile.read(reinterpret_cast<char *>(section.data.data() + offsetBefore), header.p_filesz);

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

    std::filesystem::remove(tmpFileName);

    zagtos::ProgramBinary binary = {
        .entryAddress = FileHeader.e_entry,
        .TLSSection = std::move(TLSSection),
        .sections = std::move(sections),
    };
    zbon::EncodedData output = zbon::encode(binary);

    std::ofstream outFile(argv[2]);
    if (!outFile.is_open()) {
        throw std::runtime_error("could not open output file");
    }
    assert(output.numHandles() == 0);
    outFile.write(reinterpret_cast<char *>(output.data()), output.size());
    outFile.close();
}
