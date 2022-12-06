#include "stdio-kernel.h"
#include "stdio.h"
#include "console.h"

void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vspringf(buf, format, args);
    va_end(args);
    console_put_str(buf);       // 调用console_put_str 而不是 write，因为这是内核态使用的打印函数，不能用系统调用
}
