运行`sh run.sh`运行bochs模拟器开启OS


## clang交叉编译
```shell
$ clang kernel/main.c -target i386-pc-linux-elf -c -o kernel/main.o
```


## 连接
在mac上没办法用i386的架构链接，构建i386的编译工具又好麻烦...所以索性用服务器去做了
1、 先把文件传到服务器上
```shell
$ scp kernel/main.o $server:~/os-learn
```

2、 在服务器上链接
```shell
$ ld main.o -Ttext 0xc0001500 -e main -o kernel.bin -m elf_i386
```


3、 从服务器传回到mac(记得在mac上打开远程登录的权限)
```shell
$ scp kernel.bin zhanghuanyu@192.168.3.129:/Users/zhanghuanyu/Documents/os-learn/kernel
```