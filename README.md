运行`sh run.sh`运行bochs模拟器开启OS


## 运行方式
```shell
$ make all
$ bochs -f ./bochsrc.disk
```

## 注意
在main.c中的子线程中不要再在while循环中开关中断了，直接用`console_put_str`就好
