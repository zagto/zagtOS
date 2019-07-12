#ifndef ELF_HPP
#define ELF_HPP

#include <common/common.hpp>
#include <lib/Vector.hpp>
#include <lib/Slice.hpp>

namespace elf {
    struct FileHeader32 {
        u8 ident[16];
        u16 type;
        u16 machine;
        u32 version;
        usize entry;
        usize phoff;
        usize shoff;
        u32 flags;
        u16 ehsize;
        u16 phentsize;
        u16 phnum;
        u16 shentsize;
        u16 shnum;
        u16 shstrndx;
    };

    struct FileHeader64 {
        u8 ident[16];
        u16 type;
        u16 machine;
        u32 version;
        usize entry;
        usize phoff;
        usize shoff;
        u32 flags;
        u16 ehsize;
        u16 phentsize;
        u16 phnum;
        u16 shentsize;
        u16 shnum;
        u16 shstrndx;
    };

    struct ProgramHeader32 {
        u32 type;
        usize offset;
        usize vaddr;
        usize paddr;
        u32 filesz;
        u32 memsz;
        u32 flags;
        u32 align;
    };

    struct ProgramHeader64 {
        u32 type;
        u32 flags;
        usize offset;
        usize vaddr;
        usize paddr;
        u64 filesz;
        u64 memsz;
        u64 align;
    };

    typedef ProgramHeader64 ProgramHeader;
    typedef FileHeader64 FileHeader;

    class Segment {
    private:
        Slice<Vector, u8> data;
        ProgramHeader header;

    public:
        static const u32 TYPE_LOAD{1};
        static const u32 TYPE_TLS{7};
        static const u32 FLAG_EXECUTABLE{1};
        static const u32 FLAG_WRITEABLE{2};
        static const u32 FLAG_READABLE{4};

        Segment(Slice<Vector, u8> data, ProgramHeader header) :
            data{data}, header{header} {}

        void load(UserVirtualAddress address);
        void load() {
            load(header.vaddr);
        }

        UserVirtualAddress endAddress();
        usize length() {
            return header.memsz;
        }
        UserVirtualAddress address() {
            return header.vaddr;
        }
        bool type() {
            return header.type;
        }
        Region regionInMemory();
        Permissions permissions();
    };

    class ELF {
    private:
        Slice<Vector, u8> file;
        FileHeader fileHeader;
        bool _hasTLS;
        usize tlsSegmentIndex;
        bool valid;

        ProgramHeader segmentHeader(usize index);

    public:
        ELF(Slice<Vector, u8> file);
        bool isValid();
        bool hasTLS() {
            return _hasTLS;
        }

        usize numSegments();
        Segment segment(usize index);
        UserVirtualAddress entry();
        Segment tlsSegment();
    };
}
using elf::ELF;

#endif // ELF_HPP
