#pragma once

#include <setup/HandOverState.hpp>
#include <Paging.hpp>

class ProgramBinary {
private:
    uint8_t *data;

    size_t readSize(size_t offset) const;
    size_t TLSOffset() const;
    size_t sectionsArrayOffset() const;
    size_t numSections() const;
    size_t sectionOffset(size_t index) const;
    size_t sectionAddress(size_t sectionOffset) const;
    size_t sectionSizeInMemory(size_t sectionOffset) const;
    size_t sectionFlags(size_t sectionOffset) const;
    size_t sectionDataSize(size_t sectionOffset) const;
    size_t sectionDataOffset(size_t sectionOffset) const;
    Region sectionRegion(size_t sectionOffset) const;
    void sanityCheckSection(size_t offset) const;
    void sanityChecks() const;

public:
    ProgramBinary(void *pointer);

    bool hasTLS() const;
    size_t entryAddress() const;
    void load(PagingContext pagingContext) const;
};
