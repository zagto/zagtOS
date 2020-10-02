/*#include <efi.h>
#include <util.h>
#include <elf.h>
#include <physical-memory.h>
#include <virtual-memory.h>
#include <paging.h>


static struct ElfProgramHeaderEntry *GetProgramHeader(const struct ElfFileHeader *file,
                                                      UINTN index) {
    UINTN address = (UINTN)file;
    address += file->programHeaderOffset;
    address += index * file->programHeaderEntrySize;
    return (struct ElfProgramHeaderEntry *)address;
}


UINTN LoadElfKernel(const struct ElfFileHeader *file) {
    UINTN headerIndex;
    UINTN pageIndex;
    UINTN byteIndex;
    UINTN segmentBase;
    UINTN segmentStartOffset;
    UINTN numPages;
    UINT8 *frame;
    struct ElfProgramHeaderEntry *entry;

    for (headerIndex = 0; headerIndex < file->numProgramHeaderEntries; headerIndex++) {
        entry = GetProgramHeader(file, headerIndex);
        if (entry->type == ELF_TYPE_LOAD) {
            if (!(entry->flags & ELF_READABLE)) {
                Log("Non-readable ELF segment\n");
                Halt();
            }

            segmentStartOffset = entry->virtualAddress % PAGE_SIZE;
            segmentBase = entry->virtualAddress - segmentStartOffset;
            numPages = (segmentStartOffset + entry->sizeInMemory + PAGE_SIZE - 1) / PAGE_SIZE;

            Log("Load ELF Segment to ");
            LogUINTN(entry->virtualAddress);
            Log(", ");
            LogUINTN(numPages);
            Log(" pages in memory\n");

            Log("size in file: ");
            LogUINTN(entry->sizeInFile);
            Log(", in memory ");
            LogUINTN(entry->sizeInMemory);
            Log("\n");

            for (pageIndex = 0; pageIndex < numPages; pageIndex++) {
                frame = (UINT8 *)AllocatePhysicalFrame();

                for (byteIndex = 0; byteIndex < PAGE_SIZE; byteIndex++) {
                    INTN offsetInFile = pageIndex * PAGE_SIZE + byteIndex - segmentStartOffset;

                    if (offsetInFile >= 0 && (UINTN)offsetInFile < entry->sizeInFile) {
                        frame[byteIndex] = *((UINT8 *)file + entry->offsetInFile + offsetInFile);
                    } else {
                        frame[byteIndex] = 0;
                    }
                }

                if (segmentBase + pageIndex * PAGE_SIZE < KernelImageRegion.start
                        || segmentBase + pageIndex * PAGE_SIZE > KernelPersistentDataRegion.end) {
                    Log("ELF image exceeds Kernel virtual memory region\n");
                    Halt();
                }

                MapAddress(segmentBase + pageIndex * PAGE_SIZE,
                           (EFI_PHYSICAL_ADDRESS)frame,
                           entry->flags & ELF_WRITEABLE,
                           entry->flags & ELF_EXECUTABLE,
                           FALSE,
                           CACHE_NORMAL_WRITE_BACK);
            }
        }
    }

    return file->entry;
}*/
