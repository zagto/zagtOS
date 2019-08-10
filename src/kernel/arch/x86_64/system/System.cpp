#include <paging/PagingContext.hpp>
#include <system/System.hpp>


void System::setupSecondaryProcessorEntry(BootInfo *bootInfo) {
    size_t length = static_cast<size_t>(&SecondaryProcessorEntryCodeEnd
                                        - &SecondaryProcessorEntryCode);
    size_t mptOffset = static_cast<size_t>(&SecondaryProcessorEntryMasterPageTable
                                           - &SecondaryProcessorEntryCode);
    size_t mptAddress = bootInfo->masterPageTable.value();

    assert(mptAddress < 0x100000000);
    assert(length <= PAGE_SIZE);

    cout << "loading secondary processor entry code to " << secondaryProcessorEntry.value() << endl;
    cout << "boot MPT at " << mptAddress << endl;

    uint8_t *destination = secondaryProcessorEntry.identityMapped().asPointer<uint8_t>();
    memcpy(destination, &SecondaryProcessorEntryCode, length);

    /* put the address the entry code is loaded to at offset 2 (immediate operand of mov) */
    *reinterpret_cast<uint32_t *>(destination + 2) =
            static_cast<uint32_t>(reinterpret_cast<size_t>(secondaryProcessorEntry.value()));

    *reinterpret_cast<uint32_t *>(destination + mptOffset) = static_cast<uint32_t>(mptAddress);

    /* identity-map the entry code so it continues to work when it enables paging */
    CurrentSystem.kernelOnlyPagingContext.map(UserVirtualAddress(secondaryProcessorEntry.value()),
                                              secondaryProcessorEntry,
                                              Permissions::EXECUTE);
}


System::System(BootInfo *bootInfo) :
        CommonSystem(bootInfo),
        ACPIRoot{bootInfo->ACPIRoot},
        secondaryProcessorEntry{bootInfo->secondaryProcessorEntry} {
    setupSecondaryProcessorEntry(bootInfo);
}