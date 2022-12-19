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
# - x0: virtual address of the kernel entry
# - x1: pointer to hos_v1::PagingContext structure
# - x2: kernel boot info (this pointer will be passed directly to kernel)
# - x3: temporary stack
# - x4: processorID
# - x5: hardwareID

ExitFinalize:
    # load low and high root table address into x11 and x12
    ldr x11, [x1]
    ldr x12, [x1, #8]

    # Memory Attribute Indirection Register:
    #           ++-------- type 3: write combining, TODO
    #           ||++------ type 2: TODO
    #           ||||++---- type 1: device memory, we currently always use the most restrictive type
    #           |||||              (nGnRnE)
    #           ||||||++-- type 0: normal memory
    #           ||||||||
    ldr x10, =0x000000ff

    msr mair_el1, x10

    # Translate control register:
    #                  +------------- 0b0101 - Intermediate Physical Address Size: 48bits
    #                  |+------------ 0b1101 - page size 4k, inner shareable
    #                  ||+----------- 0b0101 - outer cacheability: write back,
    #                  |||                     inner cacheability: write back
    #                  |||+---------- 0b0001 - top bit of T1SZ value 0x10: 2^(64-0x10) address space
    #                  ||||
    #                  |||| +- lower region page size 4k=00, otherwise format is the same
    #                  |||| |
    ldr x10, =0x00000000b5193519
    #ldr x10, =0x00000005b510b510

    mrs x9, id_aa64mmfr0_el1
    orr x9, x9, #0xf
    lsl x9, x9, #32
    orr x10, x10, x9
    msr tcr_el1, x10

    # set lowest bit (TTBR_CNP) in the page table addresses
    orr x11, x11, #1
    orr x12, x12, #2

    msr ttbr0_el1, x11
    msr ttbr1_el1, x12

    dsb ish
    isb

    # bits reserved to 1
    ldr x11, =0xc00800
    # bits to clear
    ldr x12, =0x308101f
    mrs x10, sctlr_el1
    orr x10, x10, x11
    bic x10, x10, x12
    orr x10, x10, #1

    msr sctlr_el1, x10

    dsb ish
    isb

    # move arguments for kernel entry and call it
    mov x10, x0
    mov x0, x2
    mov x1, x4
    mov x2, x5
    mov sp, x3

    br x10

b udff


    cmp x0, #1
    beq udff

output:

    ldr x0, =0xffffff9000000000
    ldr x1, =0xff88ffff
    ldr x2, =0xffffff00
    ldr x3, =0xffffff9000000000+0x1700000

loop:
    str x1, [x0]
    add x0, x0, 8
    cmp x0, x3
    bne loop

hang:
    b hang


udff:
    udf #1

enable_paging:

mov x9, lr


ret


