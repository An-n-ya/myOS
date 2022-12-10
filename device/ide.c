#include "ide.h"
#include "stdio-kernel.h"
#include "debug.h"
#include "io.h"
#include "timer.h"
#include "stdio.h"
#include "interrupt.h"
#include "string.h"

/**
 * 硬盘各个寄存器的端口号
 */
#define reg_data(channel)           (channel->port_base+0)
#define reg_error(channel)          (channel->port_base+1)
#define reg_sect_cnt(channel)       (channel->port_base+2)
#define reg_lba_l(channel)          (channel->port_base+3)
#define reg_lba_m(channel)          (channel->port_base+4)
#define reg_lba_h(channel)          (channel->port_base+5)
#define reg_dev(channel)            (channel->port_base+6)
#define reg_status(channel)         (channel->port_base+7)
#define reg_cmd(channel)            (reg_status(channel))
#define reg_alt_status(channel)     (channel->port_base+0x206)
#define reg_ctl(channel)            (reg_alt_status(channel))

/**
 * reg_alt_status寄存器的一些关键位
 */
#define BIT_ALT_STAT_BSY            0x80    // 硬盘忙
#define BIT_ALT_STAT_RDY            0x40    // drive ready
#define BIT_ALT_STAT_DRQ            0x8     // 数据传输准备好了

/**
 * device 寄存器的一些关键位
 */
#define BIT_DEV_MBS                 0xa0    // 第七位和第五位固定为1
#define BIT_DEV_LBA                 0x40
#define BIT_DEV_DEV                 0x10

/**
* 一些硬盘操作的指令
*/
#define CMD_IDENTIFY                0xec    // identify指令
#define CMD_READ_SECTOR             0x20    // 读扇区指令
#define CMD_WRITE_SECTOR            0x30    // 写扇区指令

/**
 * 可读写的最大扇区数
 */
#define max_lba ((80*1024*1024/512)-1)      // 只支持80M的硬盘
int32_t ext_lba_base = 0;                   // 总扩展分区的起始lba，初始化为0

uint8_t p_no = 0, l_no = 0;                 // 用于记录硬盘主分区和逻辑分区的下标
struct list partition_list;                 // 分区队列

// 分区表项目
struct partition_table_entry {
    uint8_t bootable;       // 是否可引导
    uint8_t start_head;     // 起始磁头号
    uint8_t start_sec;      // 起始扇区号
    uint8_t start_chs;      // 起始柱面号
    uint8_t fs_type;        // 分区类型
    uint8_t end_head;       // 结束磁头号
    uint8_t end_sec;        // 结束扇区号
    uint8_t end_chs;        // 结束柱面号

    uint32_t start_lba;     // 本分区起始扇区的lba地址
    uint32_t sec_cnt;       // 本分区的扇区数目
} __attribute__ ((packed)); // 禁用gcc的自动对齐，让此结构无空隙

struct boot_sector {
    uint8_t  other[446];    // 本分区起始扇区的lba地址
    struct partition_table_entry partition_table[4];// 分区表中有4项，共64字节
    uint16_t signature;     // 启动扇区的结束标志0x55 0xaa
};__attribute__((packed))


uint8_t channel_cnt;                // 按硬盘数计算的通道数
struct ide_channel channels[2];     // 两个ide通道

/**
 * 选择要读写的硬盘
 */
static void select_disk(struct disk* hd) {
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    if (hd->dev_no == 1) {
        // 如果是从盘，就置DEV位为1
        reg_device |= BIT_DEV_DEV;
    }
    outb(reg_dev(hd->my_channel), reg_device);
}

/**
 * 向硬盘控制器写入起始扇区地址以及要读写的扇区数
 * @param lba 扇区号
 */
static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt) {
    ASSERT(lba <= max_lba);
    struct ide_channel* channel = hd->my_channel;

    // 写入要读写的扇区数到reg_sect_cnt
    outb(reg_sect_cnt(channel), sec_cnt);

    // 写入lba地址, 即扇区号
    outb(reg_lba_l(channel), lba); // outb自动取低八位
    outb(reg_lba_m(channel), lba >> 8);
    outb(reg_lba_h(channel), lba >> 16);

    // lba地址的24~27位需要存储在 device 寄存器的 0~3位
    outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

/**
 * 向channel发命令cmd
 */
static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
    // 发命令时将expecting_intr置true，中断处理程序会根据它判断
    channel->expecting_intr = true;
    outb(reg_cmd(channel), cmd);
}

/**
 * 从硬盘读入sec_cnt个扇区的数据到buf
 */
static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt){
    uint32_t size_in_byte;
    if (sec_cnt == 0) {
        // 把零当做256
        size_in_byte = 256 * 512;
    } else {
        size_in_byte = sec_cnt * 512;
    }
    insw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

/**
 * 将buf中sec_cnt个扇区的内容写入硬盘
 */
static void write_to_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t size_in_byte;
    if (sec_cnt == 0) {
        // 把零当做256
        size_in_byte = 256 * 512;
    } else {
        size_in_byte = sec_cnt * 512;
    }
    outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

/**
 * 等待硬盘(如果硬盘忙的话)30秒
 */
static bool busy_wait(struct disk* hd) {
    struct ide_channel* channel = hd->my_channel;
    uint16_t time_limit = 30 * 1000;
    // 每次减10毫秒
    while (time_limit -= 10 >= 0) {
        if (!(inb(reg_status(channel)) & BIT_ALT_STAT_BSY)) {
            // 如果硬盘不忙了，查看status，判断硬盘是否准备好数据了
            return (inb(reg_status(channel)) & BIT_ALT_STAT_DRQ);
        } else {
            mtime_sleep(10);
        }
    }
    return false;
}

/**
 * 将dst中len个相邻字节交换位置后存入buf
 */
static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len) {
    uint8_t idx;
    for (idx = 0; idx < len; idx += 2) {
        // 让相邻两个元素交换位置
        buf[idx + 1] = *dst++;
        buf[idx] = *dst++;
    }
    buf[idx] = '\0';
}

/**
 * 获取硬盘参数信息
 */
static void identify_disk(struct disk* hd) {
    char id_info[512];
    select_disk(hd);
    cmd_out(hd->my_channel, CMD_IDENTIFY);
    sema_down(&hd->my_channel->disk_done);          // 发送指令后，就阻塞自己

    if (!busy_wait(hd)) {
        // 如果失败
        char error[64];
        sprintf(error, "%s identify failed\n", hd->name);
        PANIC(error);
    }
    // 读一个扇区
    read_from_sector(hd, id_info, 1);
    char buf[64];
    uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
    swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
    printk("    disk %s info:\n        SN: %s\n", hd->name, buf);
    memset(buf, 0, sizeof(buf));
    swap_pairs_bytes(&id_info[md_start], buf, md_len);
    printk("        MODULE: %s\n", buf);
    uint32_t sectors = *(uint32_t*)&id_info[60*2];
    printk("        SECTORS: %d\n", sectors);
    printk("        CAPACITY: %dMB\n", sectors * 512 / 1024 / 1024);
}

/**
 * 扫描硬盘hd中地址为ext_lba的扇区中的所有分区
 */
static void partition_scan(struct disk* hd, uint32_t ext_lba) {
    struct boot_sector* bs = sys_malloc(sizeof(struct boot_sector));
    ide_read(hd, ext_lba, bs, 1);
    uint8_t part_idx = 0;
    struct partition_table_entry* p = bs->partition_table;

    // 遍历分区表4个分区项
    while (part_idx++ < 4) {
        if (p->fs_type == 0x5) {
            // 如果是扩展分区
            if (ext_lba_base != 0) {

                partition_scan(hd, p->start_lba + ext_lba_base);
            } else {
                // ext_lba_base为0表示这是第一次读取引导块，也就是主引导记录所在的扇区
                // 后面的扩展分区地址都相对于此
                ext_lba_base = p->start_lba;
                partition_scan(hd, p->start_lba);
            }
        } else if (p->fs_type != 0)  {
            // 如果是有效的分区类型
            if (ext_lba == 0) {
                // 如果此时是主分区
                hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
                hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
                hd->prim_parts[p_no].my_disk = hd;
                list_append(&partition_list, &hd->prim_parts[p_no].part_tag);
                sprintf(hd->prim_parts[p_no].name, "%s%d", hd->name, p_no+1);
                p_no++;
                ASSERT(p_no < 4);
            } else {
                hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
                hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
                hd->logic_parts[l_no].my_disk = hd;
                list_append(&partition_list, &hd->logic_parts[l_no].part_tag);
                sprintf(hd->logic_parts[l_no].name, "%s%d", hd->name, l_no + 5); //逻辑分区从5开始
                l_no++;
                if (l_no >= 8) {
                    return;
                }
            }
        }
        p++;
    }
    sys_free(bs);
}

static bool partition_info(struct list_elem* pelem, int UNUSED) {
    struct partition* part = elem2entry(struct partition, part_tag, pelem);
    printk("    %s start_lba:0x%x, sec_cnt: 0x%x\n",part->name, part->start_lba, part->sec_cnt);
    return false;
}

/**
 * 从硬盘读取sec_cnt个扇区到buf
 */
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);

    // 1 先选择操作的设备
    select_disk(hd);

    uint32_t secs_op;           // 每次操作的扇区数
    uint32_t secs_done = 0;     // 已完成的扇区数
    while (secs_done < sec_cnt) {
        if ((secs_done + 256) <= sec_cnt) {
            secs_op = 256;
        } else {
            // 如果不足256字节，了就只操作剩下的了
            secs_op = sec_cnt - secs_done;
        }
        // 2 写入待读入的扇区数和起始扇区号
        select_sector(hd, lba + secs_done, secs_op);

        // 3 执行的命令写入 reg_cmd 寄存器
        cmd_out(hd->my_channel, CMD_READ_SECTOR); // 写入读数据命令, 此时硬盘开始工作

        // 硬盘开始工作后阻塞自己
        sema_down(&hd->my_channel->disk_done);

        // 4 检查硬盘状态是否可读
        if (!busy_wait(hd)) {
            // 如果不可读，返回错误
            char error[64];
            sprintf(error, "%s read sector %d failed!\n", hd->name, lba);
            PANIC(error);
        }

        // 5 把数据从硬盘的缓冲区中读出
        read_from_sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
        secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);
}

/**
 * 将buf中的sec_cnt个扇区的内容写入硬盘
 */
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
    ASSERT(lba <= max_lba);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);

    // 1 选择要操作的设备
    select_disk(hd);
    uint32_t secs_op;
    uint32_t secs_done = 0;
    while (secs_done < sec_cnt) {
        if ((secs_done + 256) <= sec_cnt) {
            secs_op = 256;
        } else {
            // 如果不足256字节，了就只操作剩下的了
            secs_op = sec_cnt - secs_done;
        }
        // 2 写入待读入的扇区数和起始扇区号
        select_sector(hd, lba + secs_done, secs_op);

        // 3 执行的命令写入 reg_cmd 寄存器
        cmd_out(hd->my_channel, CMD_WRITE_SECTOR); // 写入读数据命令, 此时硬盘开始工作

        // 4 检查硬盘状态是否可写
        if (!busy_wait(hd)) {
            // 如果不可读，返回错误
            char error[64];
            sprintf(error, "%s read sector %d failed!\n", hd->name, lba);
            PANIC(error);
        }

        // 5 将数据从缓冲区写入硬盘
        write_to_sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);

        // 硬盘此时开始工作
        // 这时候阻塞自己
        sema_down(&hd->my_channel->disk_done);
        secs_done += secs_op;
    }
    lock_release(&hd->my_channel->lock);
}

/**
 * 硬盘中断处理程序
 */
void intr_hd_handler(uint8_t irq_no) {
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint8_t ch_no = irq_no - 0x2e;
    // 获取当前ide channel
    struct ide_channel* channel = &channels[ch_no];
    ASSERT(channel->irq_no == irq_no);
    // cmd_out会让expecting_intr为true
    if (channel->expecting_intr) {
        channel->expecting_intr = false;
        sema_up(&channel->disk_done);

        // 读入数据，清掉中断，让硬盘继续工作
        inb(reg_status(channel));
    }
}

/**
 * 硬盘数据初始化
 */
void ide_init() {
    printk("ide_init start\n");
    uint8_t hd_cnt = *((uint8_t*)(0x475));      // 获取硬盘数量（bochs里 0x475位置存放着硬盘数量）
                                                // 因为低端1MB以内的虚拟地址和物理地址相同，所以这里可以访问到
    ASSERT(hd_cnt > 0);
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2);      // 一个ide通道上有两个硬盘，硬盘数除以2就是通道数

    struct ide_channel* channel;
    uint8_t channel_no = 0;
    uint8_t dev_no = 0;
    // 挨个处理每个通道上的所有硬盘
    while (channel_no < channel_cnt) {
        channel = &channels[channel_no];
        switch (channel_no) {
            case 0:
                channel->port_base = 0x1f0;     // ide0通道的起始端口号是0x1f0
                channel->irq_no = 0x20 + 14;    // 从片8259a上倒数第二的中断引脚
                break;
            case 1:
                channel->port_base = 0x170;     // ide1通道的起始端口号是0x170
                channel->irq_no = 0x20 + 15;    // 从片8259a上最后一个中断引脚
                break;
        }
        channel->expecting_intr = false;        // 未向硬盘写入指令时，不期待硬盘的中断
        lock_init(&channel->lock);
        // 将disk_done信号量初始化为0，向硬盘发出请求数据后，使用sema_down阻塞线程，知道硬盘完成后调用sema_up唤醒线程
        sema_init(&channel->disk_done, 0);
        // 注册中断处理程序
        // 中断号就是irq_no
        register_handler(channel->irq_no, intr_hd_handler);


        // 分别获取两个硬盘的参数和分区信息
        while (dev_no < 2) {
            struct disk* hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);
            identify_disk(hd);
            if (dev_no != 0) {
                // 不处理hd60M，这里处理了hd80M
                partition_scan(hd, 0);
            }
            p_no = 0, l_no = 0;
            dev_no++;
        }
        dev_no = 0;
        channel_no++; // 下一个channel
    }
    printk("\n    all partition info\n");
    list_traversal(&partition_list, partition_info, (int)NULL);
    printk("ide_init done\n");
}

