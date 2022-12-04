运行`sh run.sh`运行bochs模拟器开启OS


## 运行方式
```shell
$ make all
$ bochs -f ./bochsrc.disk
```

## 注意
在main.c中的子线程中不要再在while循环中开关中断了，直接用`console_put_str`就好

macOS上的bximage和linux平台下的不太一样，macOS上的bximage创建出来的镜像文件有点问题，简易用linux平台的bximage创建镜像

在使用fdisk创建分区的时候，记得用`-c=dos -u=cylinders`选项

