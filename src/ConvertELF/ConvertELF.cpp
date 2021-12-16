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

static constexpr size_t USER_STACK_SIZE = 2 * 1024 * 1024;
static constexpr size_t USER_STACK_BORDER = 0x1000 * 10;

static constexpr size_t USER_STACK_START = 0x0000800000000000 - USER_STACK_SIZE;

static constexpr size_t TLS_INFO_ADDRESS = 0x7FFFFFDF5800;
static constexpr size_t PTHREAD_SELF_SIZE = 0x800;
#else
#error "Please add page definition for arch"
#endif

struct TLSInfo {
    uint64_t masterTLSStart;
    uint64_t masterTLSLength;
    uint64_t threadTLSPointer;
};

int main(int argc, char **argv) {
    if (!(argc == 3 || (argc == 4 && strcmp(argv[3], "--no-add-stack") == 0))) {
        throw std::runtime_error("usage: ConvertELF <input> <output> [--no-add-stack] (but got "
                                 + std::to_string(argc)
                                 + " arguments instead)");
    }
    const bool addStack = argc == 3;
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
    sections.reserve(numSections + 2);

    if (addStack) {
        zagtos::ProgramSection stackSection = {
            .address = USER_STACK_START,
            .sizeInMemory = USER_STACK_SIZE,
            .flags = PF_R | PF_W,
            .data = {},
        };
        zagtos::ProgramSection borderSection = {
            .address = USER_STACK_START - USER_STACK_BORDER,
            .sizeInMemory = USER_STACK_BORDER,
            .flags = 0,
            .data = {},
        };
        sections.push_back(stackSection);
        sections.push_back(borderSection);
    }

    bool tlsFound = false;
    size_t tlsPointer = 0;

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

        inFile.seekg(header.p_offset);
        inFile.read(reinterpret_cast<char *>(section.data.data() + offsetBefore), header.p_filesz);

        if (header.p_type == PT_TLS) {
            /* page aligned area the thread copies the master TLS contents to, plus a page
             * containing addresses for this */

            /* Thread TLS needs to be at different position from master TLS */
            section.address = TLS_INFO_ADDRESS - PTHREAD_SELF_SIZE - section.sizeInMemory;

            tlsPointer = section.address + offsetBefore + header.p_memsz;
            assert(tlsPointer % 8 == 0); // TODO: what if misaligned?
            TLSInfo info = {
                .masterTLSStart = header.p_vaddr,
                .masterTLSLength = header.p_memsz,
                .threadTLSPointer = tlsPointer,
            };
            section.data.resize(section.sizeInMemory + PAGE_SIZE);
            memcpy(section.data.data() + section.sizeInMemory + PTHREAD_SELF_SIZE,
                   reinterpret_cast<uint8_t *>(&info),
                   sizeof(TLSInfo));
            /* additional room for pthread_self and TLS info */
            section.sizeInMemory += PAGE_SIZE;

            section.flags = PF_R | PF_W;

            if (tlsFound) {
                throw std::runtime_error("input file has more than one TLS section");
            }
            tlsFound = true;
        }

        std::cerr << "resulting address "<<section.address<<", size "<<section.data.size()<<", sizeInMemory "<<section.sizeInMemory<<std::endl;


        sections.push_back(std::move(section));
    }
    inFile.close();

    std::filesystem::remove(tmpFileName);

    zagtos::ProgramBinary binary = {
        .entryAddress = FileHeader.e_entry,
        .tlsPointer = tlsPointer,
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
