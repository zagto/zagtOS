#include <common/common.hpp>
#include <processes/ProcessAddressSpace.hpp>
#include <system/System.hpp>
#include <processes/MappedArea.hpp>
#include <processes/Process.hpp>
#include <log/BasicLog.hpp>

struct Identification {
    uint8_t magic[4];
    uint8_t fileClass;
    uint8_t dataEncoding;
    uint8_t fileVersion;
    uint8_t OSABIIdentification;
    uint8_t ABIVersion;
    uint8_t pad[7];
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


struct PRStatus {
    /* magic values found in bfd/elf64-x86_64.c of binutils */
    static const size_t Size = 336;
    static const size_t RegistersOffset = 112;
    static const size_t RegistersSize = 216;
    static const size_t ElementSize = sizeof(uint64_t);

    uint64_t data[Size / ElementSize];
};


struct NoteHeader {
    uint32_t namesz = 5;
    uint32_t descsz = sizeof(PRStatus);
    uint32_t type = 1; /* PRStatus */
    char name[8] = {'C','O','R','E',0,0,0,0};
};

uint8_t digitToChar(uint8_t digit) {
    return digit >= 10 ? digit - 10 + 'a' : digit + '0';
}


void writeDump(vector<uint8_t> &dumpFile, void *data, size_t length) {
    size_t offset = dumpFile.size();
    dumpFile.resize(offset + length);
    memcpy(&dumpFile[offset], data, length);
}

void ProcessAddressSpace::coreDump(Thread *crashedThread) {
    cout << "Sending Core Dump to Serial Port..." << endl;
    vector<uint8_t> dumpFile;

    scoped_lock sl(lock);

    size_t numProgramHeaders = mappedAreas.size() + 1; /* each mapped area + registers */

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
#ifdef SYSTEM_AARCH64
        .machine = 183,
#else
#error "unknown architecture"
#endif
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

    size_t dataOffset = sizeof(FileHeader) + numProgramHeaders * sizeof(ProgramHeader);
    for (size_t index = 0; index < numProgramHeaders - 1; index++) {
        size_t startAddress = mappedAreas[index]->region.start;
        size_t length = mappedAreas[index]->region.length;

        ProgramHeader programHeader = {
            .type = 1,     /* LOAD */
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

    ProgramHeader noteProgramHeader = {
        .type = 4, /* NOTE */
        .flags = 0,
        .offset = dataOffset,
        .vaddr = 0,
        .paddr = 0,
        .filesz = 12 + 8 + sizeof(PRStatus),
        .memsz = 0,
        .align = 4
    };
    writeDump(dumpFile, &noteProgramHeader, sizeof(ProgramHeader));

    for (size_t index = 0; index < numProgramHeaders - 1; index++) {
        size_t startAddress = mappedAreas[index]->region.start;
        size_t length = mappedAreas[index]->region.length;

        for (size_t address = startAddress; address < startAddress + length; address += PAGE_SIZE) {
            static uint8_t page[PAGE_SIZE];
            memset(page, 0, PAGE_SIZE);
            if (mappedAreas[index]->permissions != Permissions::INVALID) {
                try {
                    copyFromLocked(page, address, PAGE_SIZE);
                } catch(BadUserSpace &e) {
                    /* if it's not there just leave it zero */
                } catch(...) {
                    throw;
                }
            }
            writeDump(dumpFile, page, PAGE_SIZE);
        }
    }

    NoteHeader noteHeader;
    RegisterState &regs = *crashedThread->kernelStack->userRegisterState();

    PRStatus prStatus;
    memset(&prStatus, 0, sizeof(PRStatus));
    uint64_t *prRegs = prStatus.data + PRStatus::RegistersOffset / PRStatus::ElementSize;

    /* from bfd/hosts/x64-64linux.h of binutils */
    prRegs[0] = regs.r15;
    prRegs[1] = regs.r14;
    prRegs[2] = regs.r13;
    prRegs[3] = regs.r12;
    prRegs[4] = regs.rbp;
    prRegs[5] = regs.rbx;
    prRegs[6] = regs.r11;
    prRegs[7] = regs.r10;
    prRegs[8] = regs.r9;
    prRegs[9] = regs.r8;
    prRegs[10] = regs.rax;
    prRegs[11] = regs.rcx;
    prRegs[12] = regs.rdx;
    prRegs[13] = regs.rsi;
    prRegs[14] = regs.rdi;
    prRegs[15] = regs.rax; /* orig_rax */
    prRegs[16] = regs.rip;
    prRegs[17] = regs.cs;
    prRegs[18] = regs.rflags;
    prRegs[19] = regs.rsp;
    prRegs[20] = regs.ss;
    prRegs[21] = crashedThread->tlsPointer;
    prRegs[22] = 0; /* gsbase */
    prRegs[23] = 0x18|3;
    prRegs[24] = 0x18|3;
    prRegs[25] = 0x18|3;
    prRegs[26] = 0x18|3;

    writeDump(dumpFile, &noteHeader, 12);
    writeDump(dumpFile, noteHeader.name, 8);
    writeDump(dumpFile, &prStatus, sizeof(PRStatus));

    assert(dumpFile.size() == dataOffset + 12 + 8 + sizeof(PRStatus));

    vector<uint8_t> &logName = crashedThread->process->logName;
    basicLog::sendCoreDump(logName.size(), logName.data(), dumpFile.size(), dumpFile.data());

    cout << "End core dump" << endl;
}
