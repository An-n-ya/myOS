#include "memory.h"
#include "stdint.h"
#include "kernel/print.h"

#define PG_SIZE 4096 // 页大小4K

#define MEM_BITMAP_BASE 0xc009a000 // 可用的四个页框0xc009a000 0xc009b000 0xc009c000 0xc009d000
#define K_HEAP_START 0xc0100000    // 0xc0000000是虚拟地址起始地址，0x100000指跨过低端1MB内存

struct pool kernel_pool, user_pool; // 两个内存池：内核内存池，用户内存池
struct virtual_addr kernel_vaddr;   // 结构

static void mem_pool_init(uint32_t all_mem)
{
    put_str("   mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256; // 页目录项中每一项指向4K个页，一共有256个内核页目录

    uint32_t used_mem = page_table_size + 0x100000; // 跳过低端1MB内存

    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;

    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    /*计算位图长度，没八个页构成位图的一个字节*/
    // 直接除以8可能会有少量内存丢失，但是这样做可以方便做内存越界检查
    uint32_t kbm_length = kernel_free_pages / 8; // kbm - Kernel BitMap
    uint32_t ubm_length = user_free_pages / 8;   // ubm - User BitMap

    uint32_t kp_start = used_mem;                               // kernel pool start
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE; // user pool start

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length);

    put_str("   kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int((int)kernel_pool.phy_addr_start);
    put_str("\n");

    put_str("   user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int((int)user_pool.phy_addr_start);
    put_str("\n");

    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    // 虚拟地址的位图用与维护内核堆的虚拟地址，所以需要和内核内存池大小一致
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    // 虚拟的位图放在内核内存池和用户内存池之外
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}

void mem_init()
{
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t *)(0xb00));
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done\n");
}