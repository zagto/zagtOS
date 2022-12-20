KERNEL_BASE = 0xffffff8000000000;

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
        . = ALIGN(8);
        _init_array = .;
        *(.init_array)
        _init_array_end = .;
    }

    . = ALIGN(4096);
    .eh_frame :
    {
        *(.eh_frame)
    }

    . = ALIGN(4096);
    .gcc_except_table :
    {
        *(.gcc_except_table)
    }


    . = ALIGN(4096);
    .bss :
    {
        *(.bss)
        *(COMMON)
    }    

   /DISCARD/ :
   {
        *(.comment)
        *(.note*)
   }
}
