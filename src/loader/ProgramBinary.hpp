#pragma once

#include <setup/HandOverState.hpp>
#include <Paging.hpp>

class ProgramBinary {
private:
    uint8_t *data;
    Region _TLSRegion;
    UserVirtualAddress _TLSBase;

    size_t readSize(size_t offset) const;
    size_t sectionsArrayOffset() const;
    size_t sectionFlags(size_t sectionOffset) const;
    size_t sectionDataSize(size_t sectionOffset) const;
    size_t sectionDataOffset(size_t sectionOffset) const;
    Region sectionRegion(size_t sectionOffset) const;
    void sanityCheckSection(size_t offset) const;
    void sanityChecks() const;

public:
    ProgramBinary(void *pointer);

    bool hasTLS() const;
    size_t TLSOffset() const;
    size_t sectionOffset(size_t index) const;
    size_t sectionAddress(size_t sectionOffset) const;
    hos_v1::Permissions sectionPermissions(size_t sectionOffset) const;
    size_t sectionSizeInMemory(size_t sectionOffset) const;
    size_t loadedUserFrames() const;
    size_t entryAddress() const;
    UserVirtualAddress TLSBase() const;
    Region TLSRegion() const;
    UserVirtualAddress masterTLSBase() const;
    size_t numSections() const;
    UserVirtualAddress runMessageAddress() const;
    void load(PagingContext pagingContext,
              hos_v1::Frame *frames,
              size_t &frameIndex);
};
