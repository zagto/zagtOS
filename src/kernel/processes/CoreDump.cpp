#include <common/common.hpp>
#include <processes/Process.hpp>
#include <system/System.hpp>

struct Identification {
    uint8_t magic[4];
    uint8_t fileClass;
    uint8_t dataEncoding;
    uint8_t fileVersion;
    uint8_t OSABIIdentification;
    uint8_t ABIVersion;
    uint8_t pad[9];
};

struct FileHeader {
    Identification ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    size_t entry;
    size_t phoff;
    size_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t flags;
    size_t offset;
    size_t vaddr;
    size_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
};

uint8_t digitToChar(uint8_t digit) {
    return digit >= 10 ? digit - 10 + 'a' : digit + '0';
}


void writeDump(vector<uint8_t> &dumpFile, void *data, size_t length) {
    size_t offset = dumpFile.size();
    dumpFile.resize(offset + length);
    memcpy(&dumpFile[offset], data, length);
}

void Process::coreDump() {
    cout << "Sending Core Dump to Serial Port..." << endl;
    vector<uint8_t> dumpFile;

    scoped_lock sl(pagingLock);

    size_t numProgramHeaders = mappedAreas.size();

    FileHeader fileHeader = {
        .ident = {
            .magic = {0x7f, 'E', 'L', 'F'},
            .fileClass = 2,             /* 64-bit */
            .dataEncoding = 1,          /* little-endian */
            .fileVersion = 1,
            .OSABIIdentification = 0,   /* System V */
            .ABIVersion = 0,
            .pad{0}
        },
        .type = 4,      /* core dump */
#ifdef SYSTEM_X86_64
        .machine = 62,
#else
    #error "unknown architecture"
#endif
        .version = 1,
        .entry = 0,
        .phoff = sizeof(FileHeader),
        .shoff = 0,
        .flags = 0,
        .ehsize = sizeof(FileHeader),
        .phentsize = sizeof(ProgramHeader),
        .phnum = static_cast<uint16_t>(numProgramHeaders),
        .shentsize = 0,
        .shnum = 0,
        .shstrndx = 0,
    };
    writeDump(dumpFile, &fileHeader, sizeof(FileHeader));

    size_t dataOffset = sizeof(FileHeader) * numProgramHeaders * sizeof(ProgramHeader);
    for (size_t index = 0; index < numProgramHeaders; index++) {
        size_t startAddress = mappedAreas[index]->region.start;
        size_t length = mappedAreas[index]->region.length;

        ProgramHeader programHeader = {
            .type = 16,    /* LOAD */
            .flags = 0x7,  /* read/write/execute */
            .offset = dataOffset,
            .vaddr = startAddress,
            .paddr = 0,
            .filesz = length,
            .memsz = length,
            .align = 1
        };
        writeDump(dumpFile, &programHeader, sizeof(ProgramHeader));
        dataOffset += length;
    }

    for (size_t index = 0; index < numProgramHeaders; index++) {
        size_t startAddress = mappedAreas[index]->region.start;
        size_t length = mappedAreas[index]->region.length;

        for (size_t address = startAddress; address < startAddress + length; address += PAGE_SIZE) {
            static uint8_t page[PAGE_SIZE];
            if (!copyFromUser(page, address, PAGE_SIZE, false)) {
                memset(page, 0, PAGE_SIZE);
            }

            writeDump(dumpFile, page, PAGE_SIZE);
        }
    }
    cout.sendCoreDump(logName.size(), logName.data(), dumpFile.size(), dumpFile.data());

    cout << "End core dump" << endl;

}
