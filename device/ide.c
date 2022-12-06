#include "ide.h"
#include "stdio-kernel.h"
#include "debug.h"

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

uint8_t channel_cnt;                // 按硬盘数计算的通道数
struct ide_channel channels[2];     // 两个ide通道

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
        channel_no++;
    }
    printk("ide_init done\n");
}

