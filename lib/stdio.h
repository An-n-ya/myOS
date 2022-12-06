#ifndef A_STDIO_H
#define A_STDIO_H
#include "stdint.h"
#include "global.h"

#define va_start(ap, v) ap = (va_list)&v            // 把 ap 指向的第一个固定参数v
#define va_arg(ap, t) *((t*)(ap += 4))              // ap 指向下一个参数，并转换类型返回其值
#define va_end(ap)  ap = NULL                       // 清除ap

typedef char* va_list;
uint32_t printf(const char* format, ...);
uint32_t sprintf(char* buf, const char* format, ...);
uint32_t vspringf(char* str, const char* format, va_list ap);

#endif //A_STDIO_H
