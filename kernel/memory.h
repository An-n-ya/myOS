#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "kernel/bitmap.h"

enum pool_flags
{
    PF_KERNEL = 1, // 内核内存池
    PF_USER = 2    // 用户内存池
};

#define PG_P_1 1
#define PG_P_0 0
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_S 0
#define PG_US_U 4

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
void *get_kernel_pages(uint32_t pg_cnt);
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt);
static void page_table_add(void *_vaddr, void *_page_phyaddr);
static void *palloc(struct pool *m_pool);
uint32_t *pde_ptr(uint32_t vaddr);
uint32_t *pte_ptr(uint32_t vaddr);
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt);

#endif