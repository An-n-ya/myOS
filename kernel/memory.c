#include "memory.h"
#include "stdint.h"
#include "kernel/print.h"
#include "string.h"
#include "kernel/bitmap.h"
#include "debug.h"

#define PG_SIZE 4096 // 页大小4K

#define MEM_BITMAP_BASE 0xc009a000 // 可用的四个页框0xc009a000 0xc009b000 0xc009c000 0xc009d000
#define K_HEAP_START 0xc0100000    // 0xc0000000是虚拟地址起始地址，0x100000指跨过低端1MB内存

struct pool kernel_pool, user_pool; // 两个内存池：内核内存池，用户内存池
struct virtual_addr kernel_vaddr;   // 结构

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

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

/**
 * @brief 在内存池中申请pg_cnt个虚拟页, 申请成功返回虚拟页的起始地址，否则返回NULL
 *
 * @param pf
 * @param pg_cnt
 * @return void*
 */
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if (pf == PF_KERNEL)
    {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1)
        {
            return NULL;
        }
        while (cnt < pg_cnt)
        {
            // 这连续pg_cnt个位置1，表示这些页被占用
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }
        // 计算得到页开始地址
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else
    {
        // TODO: 用户内存池
    }
    return (void *)vaddr_start;
}
/**
 * @brief 得到vaddr所属的pte指针
 *
 * @param vaddr
 * @return uint32_t*
 */
uint32_t *pte_ptr(uint32_t vaddr)
{
    // 1023个pde是最后一个页目录项，最后一个页目录项保存这页目录表的物理地址
    // 1023的十六进制为0x3ff，将其移到高10位后，为0xffc00000 (这就是new_vaddr的高10位)
    // 把vaddr中的pde当做pte，也就是把vaddr的高10位取出来作为new_vaddr的中间10位
    // 把vaddr的pte左移两位作为页内索引，所以是new_vaddr的低12位

    uint32_t *pte = (uint32_t *)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + (PTE_IDX(vaddr) << 2));

    // 最后的到的就是获取pte物理地址的new_vaddr
    return pte;
}

/**
 * @brief 得到vaddr所属的pde指针
 *
 * @param vaddr
 * @return uint32_t*
 */
uint32_t *pde_ptr(uint32_t vaddr)
{
    uint32_t *pde = (uint32_t *)((0xfffff000) + (PDE_IDX(vaddr) << 2));
    return pde;
}

/**
 * @brief 向m_pool指向的内存池中分配1个物理页，如果成功就返回页的物理地址，否则返回NULL
 *
 * @param m_pool
 * @return void*
 */
static void *palloc(struct pool *m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1)
    {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    // 位索引乘以页大小 + 内存池基址    得到新分配的页物理地址
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void *)page_phyaddr;
}

/**
 * @brief 在页表中添加虚拟地址vaddr和物理地址page_phyaddr的映射
 *
 * @param _vaddr
 * @param _page_phyaddr
 */
static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t *pde = pde_ptr(vaddr); // 获取vaddr所属的pde指针
    uint32_t *pte = pte_ptr(vaddr); // 获取vaddr所属的pte指针
    // 如果页目录项的P位(Present位)为1，说明该表存在于物理地址
    if (*pde & 0x1)
    {
        // 页表项p位应该为0，表示该页表还不存在，需要创建
        ASSERT(!(*pte & 0x1));
        // 更改pte指向的位置的值
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1); // 置US RW P位为1 表示用户级, 可读可写, 存在
    }
    else
    {
        // 页目录项不存在，需要先创建页目录在创建页表
        // 从内核空间分配页框
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);

        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        // 把分配到的pde对应的物理内存清零
        memset((void *)((int)pte & 0xfffff000), 0, PG_SIZE);
        // 清零后， pte对应的p位应该是零了
        ASSERT(!(*pte & 0x1));
        // 更改pte指向的位置的值
        *pte = (page_phyaddr | PG_US_U | PG_RW_R | PG_P_1);
    }
}

/**
 * @brief 分配pg_cnt个页空间，成就就返回起始虚拟地址，否则返回NULL
 *
 * @param pf
 * @param pg_cnt
 * @return void*
 */
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    // 分三步走：
    //     1. 先在虚拟内存中申请虚拟地址
    //     2. 通过palloc在物理内存池中申请物理页
    //     3. 通过page_table_add将第一步和第二步得到的虚拟地址和物理页地址在页表中完成映射(写入页表)
    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL)
    {
        return NULL;
    }

    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    // 虚拟地址是连续的，但物理地址不一定连续，需要逐个做映射
    while (cnt-- > 0)
    {
        void *page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL)
        {
            return NULL;
        }
        page_table_add((void *)vaddr, page_phyaddr); // 在页表中做映射
        // 映射下一个虚拟地址
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}

/**
 * @brief 向内核内存池申请cg_cnt个页空间
 *
 * @param pg_cnt
 * @return void*
 */
void *get_kernel_pages(uint32_t pg_cnt)
{
    void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if (vaddr != NULL)
    {
        // 把申请到的内存清零
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    return vaddr;
}
