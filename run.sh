if [ ! -d "./out" ]; then
    # 如果out不存在就创建该文件夹
    mkdir out
fi

if [ -e "hd60M.img" ]; then
    rm -rf hd60M.img
fi

# 编译
nasm -I include/ -o ./out/mbr.bin ./boot/mbr.s

nasm -I include/ -o ./out/loader.bin ./boot/loader.s

clang kernel/main.c -target i386-pc-linux-elf -c -o kernel/main.o

# 登录服务器链接
# 先传文件
scp kernel/main.o $server:~/os-learn
# 链接
ssh $server "ld ~/os-learn/main.o -Ttext 0xc0001500 -e main -o ~/os-learn/kernel.bin -m elf_i386"
# 拷贝文件
scp $server:~/os-learn/kernel.bin kernel


# 创建hd60M.img镜像文件
bximage -q -func=create -hd=60M -imgmode=flat hd60M.img

dd if=./out/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

dd if=./out/loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc

dd if=./kernel/kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc

bochs -f bochsrc.disk
