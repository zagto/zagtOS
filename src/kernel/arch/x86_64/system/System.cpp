#include <paging/PagingContext.hpp>
#include <system/System.hpp>


hos_v1::System *_HandOverSystem;

void System::setupSecondaryProcessorEntry(const hos_v1::System &handOver) {
    size_t length = static_cast<size_t>(&SecondaryProcessorEntryCodeEnd
                                        - &SecondaryProcessorEntryCode);
    size_t mptOffset = static_cast<size_t>(&SecondaryProcessorEntryMasterPageTable
                                           - &SecondaryProcessorEntryCode);
    size_t mptAddress = handOver.handOverPagingContext.value();
    size_t entryCodeAddress = secondaryProcessorEntry.value();

    assert(mptAddress < 0x100000000);
    assert(length <= PAGE_SIZE);

    cout << "loading secondary processor entry code to " << entryCodeAddress << endl;
    cout << "boot MPT at " << mptAddress << endl;

    uint8_t *destination = secondaryProcessorEntry.identityMapped().asPointer<uint8_t>();
    memcpy(destination, &SecondaryProcessorEntryCode, length);

    /* put the address the entry code is loaded to at offset 2 (immediate operand of mov) */
    memcpy(destination + 2, &entryCodeAddress, sizeof(uint32_t));
    memcpy(destination + mptOffset, &mptAddress, sizeof(uint32_t));

    /* identity-map the entry code so it continues to work when it enables paging */
    Status status = CurrentSystem.kernelOnlyPagingContext.map(UserVirtualAddress(secondaryProcessorEntry.value()),
                                              secondaryProcessorEntry,
                                              Permissions::READ_EXECUTE,
                                              CacheType::NORMAL_WRITE_BACK);
    if (!status) {
        cout << "Exception on secondary processor entry" << endl;
        Panic();
    }
}


System::System() :
        CommonSystem(*_HandOverSystem, handOverStatus),
        ACPIRoot{_HandOverSystem->firmwareRoot},
        secondaryProcessorEntry{_HandOverSystem->secondaryProcessorEntry} {
    if (!handOverStatus) {
        cout << "Exception during System initialization" << endl;
        Panic();
    }
    /* TODO: support for Intel PCIDs could be added here */
    tlbContextsPerProcessor = 1;
    setupSecondaryProcessorEntry(*_HandOverSystem);
}
