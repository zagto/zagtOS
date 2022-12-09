.global basicInvalidate
.global basicInvalidateTLBContext
.global basicSwitchMasterPageTable

.section ".text"

# x0 - TLB Context ID (0 until PCID support)
# x1 - address
basicInvalidateTLBContext:
    b basicInvalidateTLBContext

basicInvalidate:
    b basicInvalidate

basicSwitchMasterPageTable:
    b basicSwitchMasterPageTable
