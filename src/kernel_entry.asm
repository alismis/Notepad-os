; 32-bit kernel entry stub.
; The bootloader jumps to KERNEL_OFFSET (0x1000). The linker places this
; stub first so that address is our entry point; it then calls kmain().

[bits 32]
[extern kmain]

global _start
_start:
    call kmain
.hang:
    cli
    hlt
    jmp .hang
