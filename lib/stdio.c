#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "syscall.h"
#include "global.h"

#define va_start(ap, v) ap = (va_list)&v            // 把 ap 指向的第一个固定参数v
#define va_arg(ap, t) *((t*)(ap += 4))              // ap 指向下一个参数，并返回其值
#define va_end(ap)  ap = NULL                       // 清除ap

/**
 * 将整型按进制转换成字符，也就是把整数转化成对应的ascii码字符
 */
static void itoa(uint32_t value, char** buf_ptr_addr, uint8_t base) {
    uint32_t m = value % base;          // 模
    uint32_t i = value / base;          // 取整
    if (i) {
        itoa(i, buf_ptr_addr, base);
    }
    if (m < 10) {
        // 如果余数是 0 ~ 9
        *((*buf_ptr_addr)++) = m + '0';         // 将数字0~9转化成字符'0'~'9'
    } else {
        *((*buf_ptr_addr)++) = m - 10 + 'A';    // 将数字A~F转化成字符'A'~'F'
    }
}

/**
 * 将参数 ap 按照格式 format 转化到字符串 str, 并返回替换后str的长度
 */
uint32_t vspringf(char* str, const char* format, va_list ap) {
    // 用指针指向参数
    char* buf_ptr = str;
    const char* index_ptr = format;
    char index_char = *index_ptr;
    int32_t arg_int;
    while (index_char) {
        if (index_char != '%') {
            // 如果开头不是 %，遍历下一个字符
            *(buf_ptr++) = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        // 此时开头就是%，跳过%, 获得格式类型
        // 分情况处理格式类型
        index_char = *(++index_ptr);
        switch (index_char) {
            case 'x':
                // 处理十六进制数字
                arg_int = va_arg(ap, int);      // char转成int型
                itoa(arg_int, &buf_ptr, 16);
                index_char = *(++index_ptr);    // 跳过格式类型
                break;
        }
    }

    return strlen(str);
}

uint32_t printf(const char* format, ...) {
    va_list args;
    va_start(args, format);     // 使 args 指向 format
    char buf[1024] = {0};
    vspringf(buf, format, args);
    va_end(args);
    return write(buf);

}
