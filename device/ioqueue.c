#include "ioqueue.h"
#include "../kernel/interrupt.h"
#include "../kernel/global.h"
#include "../kernel/debug.h"

/**
 * 初始化io队列
 */
void ioqueue_init(struct ioqueue* ioq) {
    lock_init(&ioq->lock);              // 初始化锁
    ioq->producer = ioq->consumer = NULL;    // 生产者和消费者置空
    ioq->head = ioq->tail = 0;               // 队尾和队首指向缓冲区数组的第0个位置
}

/**
 * 返回pos在缓冲区中的下一个位置
 */
static int32_t next_pos(int32_t pos) {
    // 用取模的方法得到下一个位置
    return (pos + 1) % bufsize;
}

/**
 * 判断队列是否已经满了
 */
bool ioq_full(struct ioqueue* ioq) {
    // 判断的时候应该关中断
    ASSERT(intr_get_status() == INTR_OFF);
    return next_pos(ioq->head) == ioq->tail;
}

/**
 * 判断队列是否为空
 */
static bool ioq_empty(struct ioqueue* ioq) {
    // 判断的时候应该关中断
    ASSERT(intr_get_status() == INTR_OFF);
    return ioq->head == ioq->tail;
}

/**
 * 使生产者/消费者阻塞等待
 * @param waiter 生产者/消费者
 */
static void ioq_wait (struct task_struct** waiter) {
    // 需要等待的task不应该为空，但是它指向的地址应该为空
    ASSERT(*waiter == NULL && waiter != NULL);
    *waiter = running_thread();
    // 阻塞当前线程
    thread_block(TASK_BLOCKED);
}

/**
 * 唤起task
 * @param waiter 生产者/消费者
 */
static void wakeup(struct task_struct** waiter) {
    ASSERT(*waiter != NULL);
    thread_unlock(*waiter);
    *waiter = NULL;
}

/**
 * 消费者从ioq队列中获取一个字符
 * @param ioq 环形队列
 * @return 一个字符
 */
char ioq_getchar(struct ioqueue* ioq) {
    // 获取的时候要关中断
    ASSERT(intr_get_status() == INTR_OFF);

    while (ioq_empty(ioq)) {
        // 如果队列为空，让消费者等着
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    char byte = ioq->buf[ioq->tail];         // 从缓冲区中取出最后一个字符
    ioq->tail = next_pos(ioq->tail);    //  队尾后移

    if (ioq->producer != NULL) {
        // 如果当前有生产者等着，就唤醒他
        wakeup(&ioq->producer);         // 环形生产者
    }

    return byte;
}

/**
 * 生产者往 ioq 队列写入一个字符
 * @param ioq   环形队列
 * @param byte  待写入的字符
 */
void ioq_putchar(struct ioqueue* ioq, char byte) {
    // 写入的时候要保证关中断
    ASSERT(intr_get_status() == INTR_OFF);

    while (ioq_full(ioq)) {
        // 如果环形队列已经满了，让生产者停下来等待
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    ioq->buf[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);        // 让队首指向下一个位置

    if (ioq->consumer != NULL) {
        // 如果当前有消费者等着，就唤醒他
        wakeup(&ioq->consumer);
    }
}



