#include "console.h"
static struct lock console_lock;        // 控制台锁

/**
 * 初始化终端
 */
void console_init() {
    lock_init(&console_lock);
}

/**
 * 获取终端
 */
static void console_acquire() {
    lock_acquire(&console_lock);
}

/**
 * 释放终端
 */
static void console_release() {
    lock_release(&console_lock);
}

/**
 * 终端输出字符串
 */
void console_put_str(char* str) {
    console_acquire();      // 获取终端锁
    put_str(str);   // 打印字符串
    console_release();      // 释放终端锁
}

/**
 * 终端输出字符
 */
void console_put_char(uint8_t char_ascii) {
    console_acquire();      // 获取终端锁
    put_char(char_ascii);   // 打印字符串
    console_release();      // 释放终端锁
}

/**
 * 中断输出十六进制整数
 */
void console_put_int(uint32_t num) {
    console_acquire();      // 获取终端锁
    put_int(num);           // 打印数字
    console_release();      // 释放终端锁
}


