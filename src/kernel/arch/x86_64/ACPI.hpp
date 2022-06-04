#pragma once

#include <common/common.hpp>


struct __attribute__((packed)) RSDPTable {
    char signature[8];
    uint8_t checksum;
    char oemID[6];
    uint8_t revision;
    uint32_t rsdtAddress;
    /* ACPI 2.0+ only: */
    uint32_t length;
    uint64_t xsdtAddress;
};

struct __attribute__((packed)) TableHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemID[6];
    char oemTableID[8];
    uint32_t oemRevision;
    char aslCompilderID[4];
    uint32_t alsCompilerRevision;
};

struct __attribute__((packed)) MADTSubtableHeader {
    uint8_t type;
    uint8_t length;
};

struct __attribute__((packed)) MADTTable : TableHeader {
    uint32_t localAPICAddress;
    uint32_t flags;
};

/* Convinent and simple but slow interface to find subtables. Good enough for kernel/loader
 * initialization. Works without heap allocations. */
template <typename T>
class SubtableWrapper {
private:
    const typename T::TableClass *table;
    size_t numMatchingSubtables;

    void scan(size_t limit, size_t &index, const uint8_t *&pointer) const noexcept {
        index = 0;
        pointer = reinterpret_cast<const uint8_t *>(table) + sizeof(typename T::TableClass);
        const uint8_t *end = reinterpret_cast<const uint8_t *>(table) + table->length;

        while (pointer < end) {
            typename T::HeaderClass header;
            memcpy(&header, pointer, sizeof(typename T::HeaderClass));

            if (header.type == T::SUBTABLE_TYPE_ID) {
                if (index == limit) {
                    break;
                }
                index++;
            }
            pointer += header.length;
        }

        if (pointer >= end) {
            pointer = nullptr;
        }
    }

public:
    SubtableWrapper(const typename T::TableClass *table) noexcept :
            table{table} {

        const uint8_t *pointer;
        /* do an unlimitied scan to fill numMatchingSubtables variable */
        scan(-1ul, numMatchingSubtables, pointer);
    }

    T operator[](size_t index) const noexcept {
        assert(index < numMatchingSubtables);
        const uint8_t *pointer;
        scan(index, index, pointer);
        assert(pointer != nullptr);

        T result;
        memcpy(&result, pointer, sizeof(T));
        return result;
    }

    size_t count() const noexcept {
        return numMatchingSubtables;
    }
};

static constexpr uint32_t ACPI_MADT_ENABLED = 1;

static constexpr uint32_t ACPI_MADT_FLAG_ACTIVE_LOW = 0x2;
static constexpr uint32_t ACPI_MADT_FLAG_LEVEL_TRIGGERED = 0x8;

struct LocalAPICSubtable : MADTSubtableHeader {
    uint8_t processorID;
    uint8_t id;
    uint32_t flags;

    /* for SubtableWrapper */
    static const uint8_t SUBTABLE_TYPE_ID = 0;
    using HeaderClass = MADTSubtableHeader;
    using TableClass = MADTTable;
};

struct IOAPICSubtable : MADTSubtableHeader {
    uint8_t id;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsiBase;

    /* for SubtableWrapper */
    static const uint8_t SUBTABLE_TYPE_ID = 1;
    using HeaderClass = MADTSubtableHeader;
    using TableClass = MADTTable;
};

struct IOAPICSourceOverride : MADTSubtableHeader {
    uint8_t busSource;
    uint8_t irqSource;
    uint32_t gsi;
    uint16_t flags;

    /* for SubtableWrapper */
    static const uint8_t SUBTABLE_TYPE_ID = 2;
    using HeaderClass = MADTSubtableHeader;
    using TableClass = MADTTable;
};


const MADTTable *findMADT(PhysicalAddress root) noexcept;
