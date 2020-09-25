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

uint8_t group[3];
size_t position = 0;
size_t linePosition = 0;

char convertToChar(uint8_t value) {
    if (value <= 25) {
        return value + 'A';
    } else if (value <= 51) {
        return value - 26 + 'a';
    } else if (value <= 61) {
        return value - 52 + '0';
    } else if (value == 62) {
        return '+';
    } else if (value == 63) {
        return '/';
    } else {
        Panic();
    }
}

void writeOutGroup() {
    cout << convertToChar(group[0] >> 2)
         << convertToChar(((group[0] & 0b11) << 4) | (group[1] >> 4))
         << convertToChar(((group[1] & 0b1111) << 2) | (group[2] >> 6))
         << convertToChar(group[2] & 0b111111);

    position = 0;
    linePosition++;
    if (linePosition > 19) {
        cout << endl;
        linePosition = 0;
    }
}

void writeBase64(void *_data, size_t length) {
    uint8_t *data = reinterpret_cast<uint8_t *>(_data);
    for (size_t index = 0; index < length; index++) {
        group[position] = data[index];
        position++;
        if (position == 3) {
            writeOutGroup();
        }
    }
}

void finalizeBase64() {
    size_t numFillBytes = 0;
    if (position > 0) {
        while (position != 3) {
            group[position] = 0;
            position++;
            numFillBytes++;
        }
        writeOutGroup();
    }
    for (size_t i = 0; i < numFillBytes; i++) {
        cout << "=";
    }
    cout << endl;

    position = 0;
    linePosition = 0;
}

void Process::coreDump() {
    cout << "===== BEGIN CORE DUMP =====" << endl;

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
    writeBase64(&fileHeader, sizeof(FileHeader));

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
        writeBase64(&programHeader, sizeof(ProgramHeader));
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

            writeBase64(page, PAGE_SIZE);
        }
    }
    finalizeBase64();

    cout << "===== BEGIN END DUMP =====" << endl;

}
