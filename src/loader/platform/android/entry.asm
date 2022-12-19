.extern _header
.extern _end
.extern _got
.extern _got_plt_end
.extern _bss
.extern _bss_end
.extern _loader_stack
.extern _init_array
.extern _init_array_end
.extern LoaderMain

.global _start
.global __cxa_atexit
.global DeviceTreeAddress

.section ".header"
_start:
    b start2
    .long 0
    # load offset
    .quad 0x80000
    # size
    .quad _end - _start
    # flags: little endian, 4k pages, physical placement anywhere
    .quad 0xa
    .quad 0
    .quad 0
    .quad 0
    .ascii "ARM\x64"

start2:
    adr x1, DeviceTreeAddress
    str x0, [x1]

    # keep relocation offset in x20
    # _start is the first byte in the image, so it's address is the relocation offset
    adr x20, _start

    # Add relocation offset to every entry in the Global Offset Table (GOT)
    adr x1, _got
    adr x2, _got_plt_end
fixGOTLoop:
    ldr x3, [x1]
    add x3, x3, x20
    str x3, [x1]

    add x1, x1, #8
    cmp x1, x2
    blt fixGOTLoop

    # clear BSS
    mov x0, #0
    adr x1, _bss
    ldr x2, =_bss_end
    add x2, x2, x20
clearBSSLoop:
    str x0, [x1]
    add x1, x1, #8
    cmp x1, x2
    blt clearBSSLoop

    # setup stack pointer
    adr x0, _loader_stack
    mov sp, x0

    # Call global constructors
    adr x21, _init_array
    adr x22, _init_array_end
initArrayLoop:
    cmp x21, x22
    beq initArrayDone
    ldr x0, [x21]
    add x0, x0, x20
    blr x0
    add x21, x21, #8
    b initArrayLoop
initArrayDone:

bl LoaderMain
# LoaderMain should never return
hang:
    wfe
    b hang

__cxa_atexit:
    # ignore __cxa_atexit
    mov x0, 0
    ret

.section ".data"
    .align 8
DeviceTreeAddress:
    .quad 0
