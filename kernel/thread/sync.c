#include "sync.h"
#include "../global.h"

/**
 * 初始化信号量
 */
void sema_init(struct semaphore* psema, uint8_t value) {
    psema->value = value;
    list_init(&psema->waiters);
}

/**
* 初始化锁
*/
void lock_init(struct lock* plock) {
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);  // 信号量初始化为1
}

/**
* 信号量 P 操作
*/
void sema_proberen(struct semaphore* psema) {
    // 开中断，保证操作原子性
    enum intr_status old_status = intr_disable();
    while (psema->value == 0) { // value小于等于0，说明锁已经被其他线程持有
        // 当前线程不应该在信号量的 waiters 队列中
        ASSERT(!list_find(&psema->waiters, &running_thread()->general_tag));
        if (list_find(&psema->waiters, &running_thread()->general_tag)) {
            PANIC("sema_down: thread blocked has been in waiters_list\n");
        }
        // 当前线程等待该锁
        list_append(&psema->waiters, &running_thread()->general_tag);
        // 阻塞当前线程
        thread_block(TASK_BLOCKED);
    }
    // 到这里的话，说明信号量大于零了
    psema->value--; // 进行P操作
    ASSERT(psema->value == 0); // 这里的信号量用作二元信号，不存在大于1的信号量
    // 恢复中断
    intr_set_status(old_status);
}

/**
* 信号量 V 操作
*/
void sema_verhogen(struct semaphore* psema) {
    // 开中断，保证操作原子性
    enum intr_status old_status = intr_disable();
    // 此时锁应该还没有释放
    ASSERT(psema->value == 0);
    // 逐个释放被锁住的线程（因为是二元信号量，所以最多只会有一个被锁住的线程）
    if (!list_empty(&psema->waiters)) {
        struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
        thread_unlock(thread_blocked);
    }
    psema->value++; // 进行V操作
    ASSERT(psema->value == 1); // 这里的信号量用作二元信号，此时信号量应该为1
    // 恢复中断
    intr_set_status(old_status);
}

/**
 * 获取锁
 */
void lock_acquire(struct lock* plock) {
    if (plock->holder != running_thread())   {
        sema_proberen(&plock->semaphore); // P操作 停掉当前线程
        plock->holder = running_thread();   // 当前线程（已经是下一个线程了）持有锁
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    } else {
        // 当前线程已经持有锁了，holder_repeat_nr加一用作记录
        plock->holder_repeat_nr++;
    }
}

/**
 * 释放锁
 */
void lock_release(struct lock* plock) {
    // 判断锁正被当前线程持有
    ASSERT(plock->holder == running_thread());
    if (plock->holder_repeat_nr > 1) {
        plock->holder_repeat_nr--; // 重复获取多少次锁，就需要重复释放多少次锁
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    plock->holder = NULL;           // 清空holder
    plock->holder_repeat_nr = 0;
    sema_verhogen(&plock->semaphore);   // V操作
}


