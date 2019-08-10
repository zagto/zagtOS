#include <tasks/Task.hpp>
#include <tasks/ELF.hpp>
#include <system/CommonSystem.hpp>
#include <lib/Slice.hpp>
#include <memory/UserSpaceObject.hpp>

using namespace elf;

ELF::ELF(Slice<vector, uint8_t> file) : file{file} {
    valid = false;

    if (file.size() <= sizeof(fileHeader)) {
        cout << "ELF: file smaller than one file header" << endl;
        return;
    }

    static const uint8_t elfMagic[]{0x7f, 'E', 'L', 'F'};
    if (!arrayCompare(file, elfMagic, 4)) {
        cout << "ELF: Invalid magic" << endl;
        return;
    }

    fileHeader = file.interpretAsObject<FileHeader>(0);

    if (file.size() <= fileHeader.phoff + fileHeader.phnum * fileHeader.phentsize
            || fileHeader.phoff > fileHeader.phoff + fileHeader.phnum * fileHeader.phentsize) {
        cout << "ELF: file does not fit all its program headers" << endl;
        return;
    }
    if (fileHeader.phentsize < sizeof(ProgramHeader)) {
        cout << "ELF: too small program header entry size" << endl;
        return;
    }

    static_assert(UserSpaceRegion.start == 0, "This code assumes user space starts at 0");
    if (!VirtualAddress::checkInRegion(UserSpaceRegion, fileHeader.entry)) {
        cout << "ELF: entry not in user space" << endl;
        return;
    }

    _hasTLS = false;
    for (size_t index = 0; index < fileHeader.phnum; index++) {
        ProgramHeader programHeader = segmentHeader(index);

        if (programHeader.type == Segment::TYPE_TLS) {
            if (_hasTLS) {
                cout << "ELF: more than one TLS segment" << endl;
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
                cout << "ELF: program header describes segment that does not fit in file" << endl;
                return;
            }
            if (programHeader.vaddr + programHeader.memsz < programHeader.vaddr) {
                cout << "ELF: Integer Overflow in memsz" << endl;
                return;
            }

            static_assert(UserSpaceRegion.start == 0, "This code assumes user space starts at 0");
            if (!VirtualAddress::checkInRegion(UserSpaceRegion,
                                               programHeader.vaddr + programHeader.memsz)) {
                cout << "ELF: segment does not fit in user space" << endl;
                return;
            }

            if ((programHeader.flags & Segment::FLAG_WRITEABLE)
                    && (programHeader.flags & Segment::FLAG_EXECUTABLE)) {
                cout << "ELF: Segment is marked as writeable and executable at the same time"
                    << endl;
                return;
            }

            /* TLS secment is expected to overlap, so don't check there */
            if (programHeader.type == Segment::TYPE_TLS) {
                continue;
            }

            for (size_t otherIndex = index + 1; otherIndex < fileHeader.phnum; otherIndex++) {
                ProgramHeader otherHeader = segmentHeader(otherIndex);
                if (otherHeader.type != Segment::TYPE_LOAD) {
                    continue;
                }

                alignedGrow(programHeader.vaddr, programHeader.memsz, PAGE_SIZE);
                alignedGrow(otherHeader.vaddr, otherHeader.memsz, PAGE_SIZE);

                /* https://stackoverflow.com/questions/3269434/whats-the-most-efficient-way-to-test-
                 * two-integer-ranges-for-overlap */
                if (programHeader.vaddr < otherHeader.vaddr + otherHeader.memsz
                        && otherHeader.vaddr < programHeader.vaddr + programHeader.memsz) {
                    cout << "ELF: Segments overlap" << programHeader.vaddr << ":" << programHeader.memsz << ", " << otherHeader.vaddr << ":" << otherHeader.memsz << endl;
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

size_t ELF::numSegments() {
    assert(valid);
    return fileHeader.phnum;
}

Segment ELF::segment(size_t index) {
    assert(valid);
    assert(index < fileHeader.phnum);

    ProgramHeader programHeader = segmentHeader(index);

    return Segment(Slice<vector, uint8_t>(&file,
                                     programHeader.offset,
                                     programHeader.filesz),
                   programHeader);
}

Segment ELF::tlsSegment() {
    assert(valid);
    assert(_hasTLS);

    return segment(tlsSegmentIndex);
}

ProgramHeader ELF::segmentHeader(size_t index) {
    return file.interpretAsObject<ProgramHeader>(fileHeader.phoff + index * fileHeader.phentsize);
}

UserVirtualAddress ELF::entry() {
    return fileHeader.entry;
}

Region Segment::regionInMemory() {
    size_t alignedAddress = header.vaddr;
    size_t alignedSize = header.memsz;
    alignedGrow(alignedAddress, alignedSize, PAGE_SIZE);

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

void Segment::load(Task *task, UserVirtualAddress address) {
    assert(header.type == TYPE_LOAD || header.type == TYPE_TLS);
    cout << "Loading Segment from " << (size_t)&data[0] << " to " << address.value() << ", size " << header.filesz << endl;

    bool valid = task->copyToUser(address.value(), &data[0], header.filesz, false);
    assert(valid);
}

UserVirtualAddress Segment::endAddress() {
    return align(header.vaddr + header.memsz, PAGE_SIZE, AlignDirection::UP);
}
