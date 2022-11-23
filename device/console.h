#ifndef OS_LEARN_CONSOLE_H
#define OS_LEARN_CONSOLE_H
#include "../lib/kernel/print.h"
#include "../lib/stdint.h"
#include "../kernel/thread/sync.h"
#include "../kernel/thread/thread.h"

void console_put_char(uint8_t char_ascii);
void console_put_str(char* str);
void console_put_int(uint32_t num);
void console_init();

#endif //OS_LEARN_CONSOLE_H
