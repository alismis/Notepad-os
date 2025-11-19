Thanks for supporting my OS 😁


.asm files are for looking at codes and for compile


you can compile .asm to .bin with 
" nasm -f bin notepadoscodefile.asm -o NotePadOS.bin " if you have v1.2- if you have v1.3+ you gonna use " nasm -f bin kernel.asm -o kernel.bin " & " nasm -f bin boot.asm -o boot.bin " code, it works in MacOS Windows Linux and Termux if you installed nasm compiler
but the last one is make them in one piece to do that in

termux cat boot.bin kernel.bin > notepados.img

in windows(CMD) copy /b boot.bin + kernel.bin notepados.img

in linux/MacOS cat boot.bin kernel.bin > notepados.img
