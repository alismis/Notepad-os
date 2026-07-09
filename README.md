# Notepad-OS

A tiny **32-bit, protected-mode** operating system that boots straight into a
notepad. Everything runs on bare metal: a hand-written bootloader enters
protected mode and hands control to a small C kernel with its own VGA and PS/2
keyboard drivers. Typing `/cpp` opens a **C++ runner** window where code you
type is executed live by a built-in interpreter.

```
Notepad-OS  -  C++ Runner
+- code.cpp ------------------------------------------------------------------+
| #include <iostream>                                                         |
| using namespace std;                                                        |
| int main() {                                                                |
|     int a = 6;                                                              |
|     int b = 7;                                                              |
|     cout << "a * b = " << a * b << endl;                                    |
|     return 0;                                                               |
| }                                                                           |
+-----------------------------------------------------------------------------+
+- output --------------------------------------------------------------------+
| a * b = 42                                                                  |
+-----------------------------------------------------------------------------+
 Write C++  |  /run = execute  /exit = back to notepad
```

## Features

- **32-bit protected mode** — the bootloader enables the A20 line, loads a flat
  GDT and switches the CPU into protected mode before jumping to the kernel.
- **Written-from-scratch drivers** (all 32-bit, no BIOS calls after boot):
  - VGA text-mode driver writing directly to `0xB8000`, with windows/boxes and
    a hardware cursor.
  - PS/2 keyboard driver polling ports `0x60`/`0x64`, with shift handling.
- **Notepad** — a multi-line text editor with scrolling.
- **`/cpp` C++ runner** — a second window with a built-in interpreter for a
  subset of C++ (`cout`, `int` variables, integer arithmetic).

## Commands

Type a command on its own line and press **Enter**.

| In notepad | Action                          |
|------------|---------------------------------|
| `/cpp`     | open the C++ runner window      |
| `/clear`   | erase the notepad               |
| `/help`    | show the built-in help          |

| In the C++ runner | Action                       |
|-------------------|------------------------------|
| `/run`            | execute the code             |
| `/exit`           | return to the notepad (or press Esc) |

## Build & run

Requires `nasm`, a 32-bit-capable `gcc` (`gcc-multilib`), `binutils` (`ld`,
`objcopy`) and `qemu`.

```sh
# Debian/Ubuntu
sudo apt-get install -y nasm gcc-multilib binutils qemu-system-x86

make          # builds build/notepados.img
make run      # boots the image in QEMU
```

You can also boot `build/notepados.img` on real hardware by writing it to a USB
stick (it is a bootable disk image) — but QEMU is strongly recommended.

## How it works

```
boot.asm         stage-1 bootloader: A20, GDT, load kernel, enter protected mode
kernel_entry.asm 32-bit entry stub that calls kmain()
kernel.c         UI, windowing, editor and command handling
drivers/vga.c    VGA text-mode driver (0xB8000)
drivers/keyboard.c PS/2 keyboard driver (port 0x60)
cpp.c            interpreter for a subset of C++
util.c           freestanding string/number helpers
linker.ld        links the kernel as a flat binary at 0x10000
```

The bootloader (sector 1) reads the kernel from the following sectors into
memory at `0x10000`, sets up segments and a stack, then jumps to the kernel's
32-bit entry point.

## About the `/cpp` interpreter

This is **not** a real C++ compiler — a full compiler and toolchain cannot run
inside a from-scratch OS of this size. It is a small interpreter that
understands a useful subset so you can genuinely type a program and watch it
run:

- `#include`, `using namespace std;`, function headers and `return` are ignored
  as boilerplate, so you can paste an ordinary-looking `main`.
- `int name = expr;` declarations and `name = expr;` assignments.
- `cout << item << item ... ;` where an item is a `"string"`, `endl`, an
  integer or an integer expression.
- integer expressions with `+ - * / %` and parentheses over `int` variables.

For real C++ compilation, cross-compile on a host machine instead.

## History

The original Notepad-OS was a 512-byte 16-bit real-mode boot sector that drew a
notepad in VGA mode `0x13`. This version is a full rewrite into 32-bit
protected mode with proper drivers and the `/cpp` runner.
