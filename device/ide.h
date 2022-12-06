#ifndef OS_LEARN_IDE_H
#define OS_LEARN_IDE_H

#include "stdint.h"
#include "sync.h"



/**
 * 分区结构
 */
struct partition {
    uint32_t start_lba;                 // 起始扇区
    uint32_t sec_cnt;                   // 扇区数
    struct disk* my_disk;               //分区所属硬盘
    struct list_elem part_tag;          // 用于队列中的标记
    char name[8];                       // 分区名称
//    struct super_block* sb;             // 分区超级块
    struct bitmap block_bitmap;         // 块位图
    struct bitmap inode_bitmap;         // inode位图
    struct list open_inodes;            // 分区已经打开的i节点队列
};

/**
 * 硬盘结构
 */
struct disk {
    char name[8];                       // 本硬盘名称
    struct ide_channel* my_channel;     // 次硬盘归属于哪个 ide 通道
    uint8_t dev_no;                     // 0-主，1-从
    struct partition prim_parts[4];     // 主分区顶多是4个
    struct partition login_parts[8];    // 逻辑分区理论上支持无限个，这里限制为最多8个
};

/**
 * ata通道结构
 */
struct ide_channel {
    char name[8];                       // ata通道名称
    uint16_t port_base;                 // 起始端口号
    uint8_t irq_no;                     // 中断号
    struct lock lock;                   // 通道锁
    bool expecting_intr;                // 等待硬盘的中断，驱动程序向硬盘发完命令后等待来自硬盘的中断
    struct semaphore disk_done;         // 这是一个信号量，用于阻塞、唤醒驱动程序
    struct disk devices[2];             // 一个通道上连接两个硬盘，一从一主
};

void ide_init();

#endif //OS_LEARN_IDE_H

