# Build the Notepad-OS 32-bit disk image.
#
#   make        -> build build/notepados.img
#   make run    -> build and boot it in QEMU
#   make clean  -> remove build artifacts
#
# Requires: nasm, a 32-bit-capable gcc (gcc-multilib), ld (binutils), qemu.

SRC        := src
BUILD      := build

CC         := gcc
LD         := ld
CFLAGS     := -m32 -ffreestanding -fno-pie -fno-stack-protector \
              -fno-builtin -nostdlib -Wall -Wextra -O2 -std=c11
LDFLAGS    := -m elf_i386 -T $(SRC)/linker.ld -nostdlib

C_SOURCES  := $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/drivers/*.c)
C_OBJECTS  := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(C_SOURCES))

IMG        := $(BUILD)/notepados.img

.PHONY: all run clean

all: $(IMG)

# Stage-1 bootloader (raw 512-byte binary).
$(BUILD)/boot.bin: $(SRC)/boot.asm
	@mkdir -p $(dir $@)
	nasm -f bin $< -o $@

# 32-bit kernel entry stub (ELF object, must link first).
$(BUILD)/kernel_entry.o: $(SRC)/kernel_entry.asm
	@mkdir -p $(dir $@)
	nasm -f elf32 $< -o $@

# C source -> ELF objects.
$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link the kernel and flatten it to a raw binary.
$(BUILD)/kernel.bin: $(BUILD)/kernel_entry.o $(C_OBJECTS) $(SRC)/linker.ld
	$(LD) $(LDFLAGS) -o $(BUILD)/kernel.elf $(BUILD)/kernel_entry.o $(C_OBJECTS)
	objcopy -O binary $(BUILD)/kernel.elf $@

# Concatenate boot sector + kernel and pad to a whole number of sectors.
$(IMG): $(BUILD)/boot.bin $(BUILD)/kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $@
	@# pad the image out to at least 49 sectors so the loader can always read
	truncate -s %512 $@
	@size=$$(stat -c %s $@); min=$$((49*512)); \
	if [ $$size -lt $$min ]; then truncate -s $$min $@; fi
	@echo "built $@"

run: $(IMG)
	qemu-system-i386 -drive format=raw,file=$(IMG),index=0,media=disk

clean:
	rm -rf $(BUILD)
