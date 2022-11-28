#ifndef OS_LEARN_SYSCALL_H
#define OS_LEARN_SYSCALL_H
#include "stdint.h"
enum SYSCALL_NR {
    SYS_GETPID
};

uint32_t getpid(void);

#endif //OS_LEARN_SYSCALL_H
