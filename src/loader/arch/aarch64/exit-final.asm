.global ExitFinalize

.section ".text"


# ExitFinalize - this function does:
# - setup Non-Execute support
# - switch to a temporary stack that is part of the loader data and therefore stay mapped during
#   the jump into kernel
# - switch to a temporary GDT for the same reason
# - activate the new master page table to map the kernel to it's correct virtual address
# - jump to kernel entry
#
# All data structures are defined in C code and pointers will be given as parameters to this
# function. This is necessary because GNU EFI's relocation hacks won't work in this assempbly code.
#
# Parameters:
# - rdi: virtual address of the kernel entry
# - rsi: master page table to activate
# - rdx: kernel boot info (this pointer will be passed directly to kernel)
# - rcx: temporary stack
# - r8: processorID
# - r9: hardwareID

ExitFinalize:
hang:
    b hang
