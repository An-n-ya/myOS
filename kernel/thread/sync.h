#ifndef OS_LEARN_SYNC_H
#define OS_LEARN_SYNC_H
#include "../../lib/kernel/list.h"
#include "../../lib/stdint.h"
#include "thread.h"
#include "../interrupt.h"
#include "../debug.h"

// 信号量结构
struct semaphore {
    uint8_t value;
    struct list waiters;
};

// 锁结构
struct lock {
    struct task_struct* holder;     // 锁的持有者
    struct semaphore semaphore;     // 当前锁的信号量
    uint32_t holder_repeat_nr;      // 锁的持有者重复申请锁的次数
};

void lock_init(struct lock* plock);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);
#endif //OS_LEARN_SYNC_H
