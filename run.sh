if [ ! -d "./out" ]; then
    # 如果out不存在就创建该文件夹
    mkdir out
fi

if [ -e "hd60M.img" ]; then
    echo "123"
    rm -rf hd60M.img
fi

# 编译
nasm -I include/ -o ./out/mbr.bin mbr.s

nasm -I include/ -o ./out/loader.bin loader2.s

# 创建hd60M.img镜像文件
bximage -q -func=create -hd=60M -imgmode=flat hd60M.img

dd if=./out/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

dd if=./out/loader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc

bochs -f bochsrc.disk
