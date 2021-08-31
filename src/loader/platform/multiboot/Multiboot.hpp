#pragma once

#include <common/common.hpp>
#include <optional>

struct __attribute__((packed)) Tag {
    uint32_t type;
    uint32_t size;
};

struct __attribute__((packed)) MemoryMapTag : Tag {
    static constexpr uint32_t EXPECTED_TYPE = 6;

    struct __attribute__((packed)) Entry {
        uint64_t baseAddress;
        uint64_t length;
        uint32_t type;

        optional<Region> usableRegion() const {
            if (type == 1) {
                /* available RAM type */
                Region region(baseAddress, length);
                alignedShrink(region.start, region.length, PAGE_SIZE);
                if (region.length > 0) {
                    return region;
                } else {
                    return {};
                }
            } else {
                return {};
            }
        }
    };

    uint32_t entrySize;
    uint32_t entryVersion;

    size_t numEntries() const {
        return (size - sizeof(MemoryMapTag)) / entrySize;
    }

    Entry *getEntry(size_t index) {
        return reinterpret_cast<Entry *>(reinterpret_cast<size_t>(this + 1) + index * entrySize);
    }
};

struct __attribute__((packed)) FramebufferTag : Tag {
    static constexpr uint32_t EXPECTED_TYPE = 8;
    uint64_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t framebufferType;
    uint16_t reserved;

    /* color info */
    uint8_t redFieldPosition;
    uint8_t redMaskSize;
    uint8_t greenFieldPosition;
    uint8_t greenMaskSize;
    uint8_t blueFieldPosition;
    uint8_t blueMaskSize;
};

struct __attribute__((packed)) ModuleTag : Tag {
    static constexpr uint32_t EXPECTED_TYPE = 3;
    uint32_t startAddress;
    uint32_t endAddress;
};

struct __attribute__((packed)) OldACPITag : Tag {
    static constexpr uint32_t EXPECTED_TYPE = 14;
};

struct __attribute__((packed)) NewACPITag : Tag {
    static constexpr uint32_t EXPECTED_TYPE = 15;
};

struct __attribute__((packed)) MultibootInfoClass {
    uint32_t totalSize;
    uint32_t reserved;
    Tag firstTag;

    template<typename TagType> TagType *getTag(size_t index) {
        Tag *tag = reinterpret_cast<Tag *>(reinterpret_cast<size_t>(this) + 8);
        size_t currentIndex = 0;
        while (tag->type != 0) {
            if (tag->type == TagType::EXPECTED_TYPE) {
                if (currentIndex == index) {
                    return static_cast<TagType *>(tag);
                }
                currentIndex++;
            }
            tag = reinterpret_cast<Tag *>(reinterpret_cast<size_t>(tag) + align(tag->size, 8, AlignDirection::UP));
        }
        return nullptr;
    }
};

extern MultibootInfoClass *MultibootInfo;
