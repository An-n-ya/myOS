if [ ! -d "./out" ]; then
    # 如果out不存在就创建该文件夹
    mkdir out
fi

if [ -e "hd60M.img" ]; then
    rm -rf hd60M.img
fi

# 清除编译产物
rm -f out/*.bin out/*.o bochsout.txt


# 编译
nasm -I include/ -o ./out/mbr.bin ./boot/mbr.s

nasm -I include/ -o ./out/loader.bin ./boot/loader.s

nasm -f elf -o out/print.o lib/kernel/print.s
nasm -f elf -o out/kernel.o kernel/kernel.s

clang -I out/ -I lib/ -I kernel/ -c -fno-builtin -o out/main.o kernel/main.c  -target i386-pc-linux-elf
clang -I out/ -I lib/ -I kernel/ -c -fno-builtin -o out/interrupt.o kernel/interrupt.c  -target i386-pc-linux-elf
clang -I out/ -I lib/ -I kernel/ -c -fno-builtin -o out/init.o kernel/init.c  -target i386-pc-linux-elf


x86_64-elf-ld -Ttext 0xc0001500 -e main -o out/kernel.bin  out/main.o out/init.o out/interrupt.o out/print.o out/kernel.o -m elf_i386

# 创建hd60M.img镜像文件
bximage -q -func=create -hd=60M -imgmode=flat hd60M.img

dd if=out/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

dd if=out/loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc

dd if=out/kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc

bochs -f bochsrc.disk
