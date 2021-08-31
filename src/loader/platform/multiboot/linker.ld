/* https://wiki.osdev.org/Creating_a_64-bit_kernel */

ENTRY(_start)
SECTIONS
{
    . = 1M;
    .multiboot :
    {
        _multiboot = .;
        *(.multiboot)
        . = ALIGN(4096);
    }

    .text :
    {
        _text = .;
        *(.text)
        *(.rodata*)
        . = ALIGN(4096);
    }

   .data :
   {
        _data = .;
        *(.data)
        . = ALIGN(4096);
   }

   .eh_frame :
   {
       _ehframe = .;
       *(.eh_frame)
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
   }

   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}
