#ifndef OS_LEARN_SYSCALL_INIT_H
#define OS_LEARN_SYSCALL_INIT_H

#include "stdint.h"

void syscall_init(void);
uint32_t sys_getpid(void);
uint32_t sys_write(char* str);
#endif //OS_LEARN_SYSCALL_INIT_H
