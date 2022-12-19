/* https://wiki.osdev.org/Creating_a_64-bit_kernel */

ENTRY(_start)
SECTIONS
{
    . = 0;
    .header :
    {
        _header = .;
        *(.header)
        . = ALIGN(4096);
    }

    .text :
    {
        _text = .;
        *(.text*)
        *(.rodata*)
        . = ALIGN(4096);
    }

   .data :
   {
        _data = .;
        *(.data)
        . = ALIGN(8);
        _init_array = .;
        *(.init_array)
        _init_array_end = .;
        . = ALIGN(4096);
        . += 4096;
        _loader_stack = .;
   }

   .eh_frame :
   {
       _ehframe = .;
       *(.eh_frame)
        . = ALIGN(4096);
   }

   .got :
   {
       _got = .;
       *(.got)
       *(.got.plt)
       _got_plt_end = .;
        . = ALIGN(4096);
   }

   .bss :
   {
       _bss = .;
       *(.bss)

       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */

        *(COMMON)
       . = ALIGN(4096);
       _bss_end = .;
   }

   _end = .;

   /DISCARD/ :
   {
        *(.comment)
        *(.symtab)
        *(.strtab)
        *(.shstrtab)
   }
}
