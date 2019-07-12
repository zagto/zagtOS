KERNEL_BASE = 0xffff800000000000;

ENTRY(KernelEntry)
SECTIONS
{	
    . = KERNEL_BASE;
    .text :
    {
        *(.text*)
    }

    . = ALIGN(4096);
    .rodata :
    {
        *(.rodata*)
    }

    . = ALIGN(4096);
    .data :
    {
        *(.data*)
    }

    . = ALIGN(4096);
    .eh_frame :
    {
        *(.eh_frame)
    }

    . = ALIGN(4096);
    .bss :
    {
        *(.bss)
        *(COMMON)
    }
    
    . = 0xffffa00000000000;
    mparams = .;
    . += 30 * 8;
    _gm_ = .;
    . += 270 * 8;
    CurrentSystem = .;
    . += 0x1000 - 300 * 8;
    _kernelHeapStart = .;
    

   /DISCARD/ :
   {
        *(.comment)
        *(.note*)
   }
}
