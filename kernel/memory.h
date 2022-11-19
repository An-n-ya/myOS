#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "kernel/bitmap.h"
struct virtual_addr
{
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

struct pool
{
    struct bitmap pool_bitmap; // 内存池的位图结构
    uint32_t phy_addr_start;   // 内存池管理物理内存的起始地址
    uint32_t pool_size;        // 内存池的字节容量
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);

#endif