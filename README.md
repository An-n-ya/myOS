运行`sh run.sh`运行bochs模拟器开启OS


## clang交叉编译
```shell
$ clang kernel/main.c -target i386-pc-linux-elf -c -o kernel/main.o
```

## 使用x86_64-elf-ld链接
安装x86的gcc
```shell]
$ brew install x86_64-elf-gcc
```

## 注意
- rd_disk_m_32:函数
    由于是在保护模式下读取，下面的代码有点不一样（使用ebx， [ds:ebx]
```asm
.go_on_read:
	in ax, dx
	mov [ds:ebx], ax	; 这里和16位的不同
	add ebx, 2 			; 每读入2个字节，bx所指向的地址便+2
	loop .go_on_read
	ret
```

- 注意main.c的位置