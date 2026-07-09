; Notepad-OS stage-1 bootloader (512 bytes).
; Runs in 16-bit real mode, loads the 32-bit C kernel from disk,
; enables the A20 line, sets up a flat GDT and enters protected mode.

[bits 16]
[org 0x7c00]

KERNEL_SEG equ 0x1000           ; segment where the kernel is loaded (0x10000)
KERNEL_OFFSET equ 0x10000       ; linear load / link address of the kernel
KERNEL_SECTORS equ 48           ; sectors of kernel to read from disk

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00              ; stack grows down from the bootloader
    mov [boot_drive], dl        ; BIOS leaves the boot drive in dl
    sti

    call load_kernel
    call enable_a20

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1                 ; set PE (protection enable)
    mov cr0, eax
    jmp CODE_SEG:protected_mode ; far jump flushes the pipeline

; ---- load kernel sectors into memory at KERNEL_SEG:0 (0x10000) ----
load_kernel:
    mov ax, KERNEL_SEG
    mov es, ax
    xor bx, bx                 ; es:bx = 0x1000:0x0000 = 0x10000
    mov ah, 0x02               ; BIOS read sectors
    mov al, KERNEL_SECTORS
    mov ch, 0x00               ; cylinder 0
    mov dh, 0x00               ; head 0
    mov cl, 0x02               ; start at sector 2 (sector 1 is the boot sector)
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    xor ax, ax
    mov es, ax
    ret

disk_error:
    mov si, msg_disk_error
.print:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0e
    int 0x10
    jmp .print
.halt:
    cli
    hlt
    jmp .halt

; ---- fast A20 enable via system control port 0x92 ----
enable_a20:
    in al, 0x92
    or al, 0x02
    out 0x92, al
    ret

; ---- 32-bit entry ----
[bits 32]
protected_mode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp
    jmp KERNEL_OFFSET          ; hand control to the kernel

; ---- flat GDT: null, 4GB code, 4GB data ----
gdt_start:
    dq 0x0
gdt_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

boot_drive:      db 0
msg_disk_error:  db "Disk read error", 0

times 510 - ($ - $$) db 0
dw 0xaa55
