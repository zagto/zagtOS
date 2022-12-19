.global basicInvalidate
.global basicInvalidateTLBContext
.global basicSwitchMasterPageTable

.section ".text"

# x0 - TLB Context ID (ASID)
# x1 - address
basicInvalidateTLBContext:
    # TODO
    tlbi vmalle1
    ret

basicInvalidate:
    # TODO
    tlbi vmalle1
    ret

basicSwitchMasterPageTable:
    # set lowest bit (TTBR_CNP) in the page table addresses
    orr x0, x0, #1
    msr ttbr0_el1, x0

    # TODO
    tlbi vmalle1

    dsb ish
    isb

    ret

