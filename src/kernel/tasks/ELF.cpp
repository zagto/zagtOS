#include <tasks/Task.hpp>
#include <tasks/ELF.hpp>
#include <system/System.hpp>
#include <lib/Slice.hpp>
#include <memory/UserSpaceWindow.h>

using namespace elf;

ELF::ELF(Slice<Vector, u8> file) : file{file} {
    valid = false;

    if (file.size() <= sizeof(fileHeader)) {
        Log << "ELF: file smaller than one file header" << EndLine;
        return;
    }

    static const u8 elfMagic[]{0x7f, 'E', 'L', 'F'};
    if (!arrayCompare(file, elfMagic, 4)) {
        Log << "ELF: Invalid magic" << EndLine;
        return;
    }

    fileHeader = file.interpretAsObject<FileHeader>(0);

    if (file.size() <= fileHeader.phoff + fileHeader.phnum * fileHeader.phentsize
            || fileHeader.phoff > fileHeader.phoff + fileHeader.phnum * fileHeader.phentsize) {
        Log << "ELF: file does not fit all its program headers" << EndLine;
        return;
    }
    if (fileHeader.phentsize < sizeof(ProgramHeader)) {
        Log << "ELF: too small program header entry size" << EndLine;
        return;
    }

    static_assert(UserSpaceRegion.start == 0, "This code assumes user space starts at 0");
    if (!VirtualAddress::checkInRegion(UserSpaceRegion, fileHeader.entry)) {
        Log << "ELF: entry not in user space" << EndLine;
        return;
    }

    _hasTLS = false;
    for (usize index = 0; index < fileHeader.phnum; index++) {
        ProgramHeader programHeader = segmentHeader(index);

        if (programHeader.type == Segment::TYPE_TLS) {
            if (_hasTLS) {
                Log << "ELF: more than one TLS segment" << EndLine;
                return;
            }
            _hasTLS = true;
            tlsSegmentIndex = index;
        }
        if (programHeader.type == Segment::TYPE_LOAD
                || programHeader.type == Segment::TYPE_TLS) {
            if (programHeader.filesz != 0 && (
                        programHeader.offset + programHeader.filesz > file.size()
                        || programHeader.offset > programHeader.offset + programHeader.filesz)) {
                Log << "ELF: program header describes segment that does not fit in file" << EndLine;
                return;
            }
            if (programHeader.vaddr + programHeader.memsz < programHeader.vaddr) {
                Log << "ELF: Integer Overflow in memsz" << EndLine;
                return;
            }

            static_assert(UserSpaceRegion.start == 0, "This code assumes user space starts at 0");
            if (!VirtualAddress::checkInRegion(UserSpaceRegion,
                                               programHeader.vaddr + programHeader.memsz)) {
                Log << "ELF: segment does not fit in user space" << EndLine;
                return;
            }

            if ((programHeader.flags & Segment::FLAG_WRITEABLE)
                    && (programHeader.flags & Segment::FLAG_EXECUTABLE)) {
                Log << "ELF: Segment is marked as writeable and executable at the same time"
                    << EndLine;
                return;
            }

            /* TLS secment is expected to overlap, so don't check there */
            if (programHeader.type == Segment::TYPE_TLS) {
                continue;
            }

            for (usize otherIndex = index + 1; otherIndex < fileHeader.phnum; otherIndex++) {
                ProgramHeader otherHeader = segmentHeader(otherIndex);
                if (otherHeader.type != Segment::TYPE_LOAD) {
                    continue;
                }

                alignedGrow(&programHeader.vaddr, &programHeader.memsz, PAGE_SIZE);
                alignedGrow(&otherHeader.vaddr, &otherHeader.memsz, PAGE_SIZE);

                /* https://stackoverflow.com/questions/3269434/whats-the-most-efficient-way-to-test-
                 * two-integer-ranges-for-overlap */
                if (programHeader.vaddr < otherHeader.vaddr + otherHeader.memsz
                        && otherHeader.vaddr < programHeader.vaddr + programHeader.memsz) {
                    Log << "ELF: Segments overlap" << programHeader.vaddr << ":" << programHeader.memsz << ", " << otherHeader.vaddr << ":" << otherHeader.memsz << EndLine;
                    return;
                }
            }
        }
    }

    valid = true;
}

bool ELF::isValid() {
    return valid;
}

usize ELF::numSegments() {
    Assert(valid);
    return fileHeader.phnum;
}

Segment ELF::segment(usize index) {
    Assert(valid);
    Assert(index < fileHeader.phnum);

    ProgramHeader programHeader = segmentHeader(index);

    return Segment(Slice<Vector, u8>(&file,
                                     programHeader.offset,
                                     programHeader.filesz),
                   programHeader);
}

Segment ELF::tlsSegment() {
    Assert(valid);
    Assert(_hasTLS);

    return segment(tlsSegmentIndex);
}

ProgramHeader ELF::segmentHeader(usize index) {
    return file.interpretAsObject<ProgramHeader>(fileHeader.phoff + index * fileHeader.phentsize);
}

UserVirtualAddress ELF::entry() {
    return fileHeader.entry;
}

Region Segment::regionInMemory() {
    usize alignedAddress = header.vaddr;
    usize alignedSize = header.memsz;
    alignedGrow(&alignedAddress, &alignedSize, PAGE_SIZE);

    return Region(alignedAddress, alignedSize);
}

Permissions Segment::permissions() {
    if (header.flags & FLAG_EXECUTABLE) {
        return Permissions::EXECUTE;
    } else if (header.flags & FLAG_WRITEABLE) {
        return Permissions::WRITE;
    } else {
        return Permissions::NONE;
    }
}

void Segment::load(UserVirtualAddress address) {
    Assert(header.type == TYPE_LOAD || header.type == TYPE_TLS);
    Log << "Loading Segment to " << address.value() << EndLine;

    UserSpaceWindow win(address.value(), header.filesz);
    arrayCopy(win, data, header.filesz);
}

UserVirtualAddress Segment::endAddress() {
    return align(header.vaddr + header.memsz, PAGE_SIZE, AlignDirection::UP);
}
