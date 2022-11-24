#include "keyboard.h"
#include "../lib/kernel/print.h"
#include "../kernel/interrupt.h"
#include "../kernel/io.h"
#include "../kernel/global.h"

#define KBD_BUF_PORT 0x60       // 键盘buffer寄存器端口号

// 键盘中断处理程序
static void intr_keyboard_handler(void) {
    uint8_t scancode = inb(KBD_BUF_PORT); // 从缓冲区读出数据，否则8042将不再响应键盘中断
    put_int(scancode);
    return;
}

void keyboard_init() {
    put_str("keyboard init start\n");
    register_handler(0x21, intr_keyboard_handler); // 注册0x21号中断（键盘中断）
    put_str("keyboard init done\n");
}



