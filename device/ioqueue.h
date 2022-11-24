#ifndef OS_LEARN_IOQUEUE_H
#define OS_LEARN_IOQUEUE_H
#include "stdint.h"
#include "../kernel/thread/thread.h"
#include "../kernel/thread/sync.h"

#define bufsize 64                      // 缓冲区大小为64

// 环形队列
struct ioqueue {
    struct lock lock;
    struct task_struct* producer;       // 生产者
    struct task_struct* consumer;       // 消费者
    char buf[bufsize];                  // 缓冲区大小
    uint32_t head;                      // 队首
    uint32_t tail;                      // 队尾
};

bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
void ioqueue_init(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);


#endif //OS_LEARN_IOQUEUE_H
