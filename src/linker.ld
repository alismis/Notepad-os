/* Link the kernel as a flat binary loaded at 0x10000.
   kernel_entry.o must come first so _start sits at 0x10000. */
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text._start)
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
}
