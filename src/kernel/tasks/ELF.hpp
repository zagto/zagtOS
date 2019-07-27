#ifndef ELF_HPP
#define ELF_HPP

#include <common/common.hpp>
#include <lib/vector.hpp>
#include <lib/Slice.hpp>

class Task;

namespace elf {
    struct FileHeader32 {
        uint8_t ident[16];
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

    struct FileHeader64 {
        uint8_t ident[16];
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

    struct ProgramHeader32 {
        uint32_t type;
        size_t offset;
        size_t vaddr;
        size_t paddr;
        uint32_t filesz;
        uint32_t memsz;
        uint32_t flags;
        uint32_t align;
    };

    struct ProgramHeader64 {
        uint32_t type;
        uint32_t flags;
        size_t offset;
        size_t vaddr;
        size_t paddr;
        uint64_t filesz;
        uint64_t memsz;
        uint64_t align;
    };

    typedef ProgramHeader64 ProgramHeader;
    typedef FileHeader64 FileHeader;

    class Segment {
    private:
        Slice<vector, uint8_t> data;
        ProgramHeader header;

    public:
        static const uint32_t TYPE_LOAD{1};
        static const uint32_t TYPE_TLS{7};
        static const uint32_t FLAG_EXECUTABLE{1};
        static const uint32_t FLAG_WRITEABLE{2};
        static const uint32_t FLAG_READABLE{4};

        Segment(Slice<vector, uint8_t> data, ProgramHeader header) :
            data{data}, header{header} {}

        void load(Task *task, UserVirtualAddress address);
        void load(Task *task) {
            load(task, header.vaddr);
        }

        UserVirtualAddress endAddress();
        size_t length() {
            return header.memsz;
        }
        UserVirtualAddress address() {
            return header.vaddr;
        }
        uint32_t type() {
            return header.type;
        }
        Region regionInMemory();
        Permissions permissions();
    };

    class ELF {
    private:
        Slice<vector, uint8_t> file;
        FileHeader fileHeader;
        bool _hasTLS;
        size_t tlsSegmentIndex;
        bool valid;

        ProgramHeader segmentHeader(size_t index);

    public:
        ELF(Slice<vector, uint8_t> file);
        bool isValid();
        bool hasTLS() {
            return _hasTLS;
        }

        size_t numSegments();
        Segment segment(size_t index);
        UserVirtualAddress entry();
        Segment tlsSegment();
    };
}
using elf::ELF;

#endif // ELF_HPP
