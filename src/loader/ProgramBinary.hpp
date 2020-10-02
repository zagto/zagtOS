#ifndef ELF_H
#define ELF_H

#include <efi.h>

struct ElfFileHeader {
    UINT8 identification[16];
    UINT16 type;
    UINT16 machine;
    UINT32 version;
    UINT64 entry;
    UINT64 programHeaderOffset;
    UINT64 sectionHeaderOffset;
    UINT32 flags;
    UINT16 elfHeaderSize;
    UINT16 programHeaderEntrySize;
    UINT16 numProgramHeaderEntries;
    UINT16 sectionHeaderEntrySize;
    UINT16 numSectionHeaders;
    UINT16 sectionNameStringTableIndex;
};

struct ElfProgramHeaderEntry {
    UINT32 type;
    UINT32 flags;
    UINT64 offsetInFile;
    UINT64 virtualAddress;
    UINT64 reserved;
    UINT64 sizeInFile;
    UINT64 sizeInMemory;
    UINT64 alignment;
};

struct LoadedElfKernel {
    UINTN minAddress;
    UINTN maxAddress;
    UINTN entry;
};

#define ELF_TYPE_LOAD 1
#define ELF_EXECUTABLE 0x1
#define ELF_WRITEABLE  0x2
#define ELF_READABLE   0x4

UINTN LoadElfKernel(const struct ElfFileHeader *file);

#endif // ELF_H
