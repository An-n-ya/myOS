#include "fs.h"
#include "ide.h"
#include "global.h"
#include "inode.h"
#include "dir.h"
#include "stdio-kernel.h"
#include "string.h"
#include "debug.h"

/**
 * 格式化分区
 *  文件系统的布局如下：
 *      #######################################################################
 *      # 引导块 # 超级块 # 空闲块位图 # inode位图 # inode数组 # 根目录 #   空闲块   #
 *      #######################################################################
 *          1       1
 */
static void partition_format(struct partition* part) {
    uint32_t boot_sector_sects = 1;     // 引导块占用的扇区数
    uint32_t super_block_sects = 1;     // 超级块占用的扇区数
    // inode位图占据的扇区数
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);    // 结果是1
    // inode数组占据的扇区数
    uint32_t inode_table_sects = DIV_ROUND_UP((sizeof (struct inode) * MAX_FILES_PER_PART), SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
    uint32_t free_sects = part->sec_cnt - used_sects;   // 总扇区数减用掉的扇区数，等于剩下的扇区数

    // 块位图占据的扇区数
    uint32_t block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
    uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;            // 空闲块位图长度
    block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);   // 空闲块最终占用的扇区数

    // 超级块初始化
    struct super_block sb;
    sb.magic = 0x19720106;                  // 魔数
    sb.sec_cnt = part->sec_cnt;             // 扇区总数
    sb.inode_cnt = MAX_FILES_PER_PART;      // inode总数
    sb.part_lba_base = part->start_lba;     // 本分区的起始lba

    // 依次设置超级块的每个lba

    // 空闲块位图
    // 空闲块位图的起始位置 = 起始lba位置 + 系统引导块大小 + 超级块位图的大小
    sb.block_bitmap_lba = sb.part_lba_base + boot_sector_sects + super_block_sects;
    sb.block_bitmap_sects = block_bitmap_sects;

    // inode位图
    // inode位图的起始位置 = 空闲块位图的起始位置 + 空闲块位图的扇区数
    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects; // 块
    sb.inode_bitmap_sects = inode_bitmap_sects;

    // inode数组
    // inode数组起始位置 = inode位图起始位置 + indoe位图的扇区数
    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
    sb.inode_table_sects = inode_table_sects;

    // 数据块
    sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
    sb.root_inode_no = 0; // 根目录的inode为0
    sb.dir_entry_size = sizeof(struct dir_entry); // 目录项的大小

    printk("%s info: \n", part->name);
    // 打印分区信息
    printk("    magic:0x%x\n", sb.magic);   // 魔数
    printk("    part_lba_base:0x%x\n", sb.part_lba_base);
    printk("    block_bitmap_lba:0x%x\n", sb.block_bitmap_lba);
    printk("    block_bitmap_sects:0x%x\n", sb.block_bitmap_sects);
    printk("    inode_bitmap:0x%x\n", sb.inode_bitmap_lba);
    printk("    inode_bitmap_sects:0x%x\n", sb.inode_bitmap_sects);
    printk("    inode_table_lba:0x%x\n", sb.inode_table_lba);
    printk("    inode_table_sects:0x%x\n", sb.inode_table_sects);
    printk("    data_start_lba:0x%x\n", sb.data_start_lba);

    // 书上的函数头有问题，没有hd的参数
    struct disk* hhd = part->my_disk;

    /***************************************
     *           1 写入超级块                *
     ***************************************/
    // 将超级块写入本分区的第一个扇区(第零个扇区是引导扇区，不用管)
    ide_write(hhd, part->start_lba + boot_sector_sects, &sb, 1);
    printk("    super_block_lba:0x%x\n", part->start_lba + boot_sector_sects);

    // 找出数据量最大的元信息，用它的尺寸作为缓冲区尺寸
    uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects \
        ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
    buf_size = (buf_size >= sb.inode_table_sects \
        ? buf_size : sb.inode_table_sects) * SECTOR_SIZE; // 最大扇区数乘以扇区大小

    uint8_t* buf = (uint8_t*) sys_malloc(buf_size);


    /***************************************
     *         2 初始化空闲块位图并写入        *
     ***************************************/
    buf[0] |= 0x01; // 第0块留给根目录
    // 由于块位图可能不正好是4096的倍数，因此位图的有些位不对应具体的物理块，需要将位图的这些位置1
    uint32_t block_bitmap_last_byte = block_bitmap_sects / 8;
    uint8_t block_bitmap_last_bit = block_bitmap_sects % 8;
    uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);
    // 将这些位置1
    memset(&buf[block_bitmap_last_byte], 0xff, last_size);
    // 上一步是按8位置1的，可能有些位被误设置为1了，现在把这些位重新置为0
    uint8_t bit_idx = 0;
    while (bit_idx <= block_bitmap_last_bit) {
        buf[block_bitmap_last_byte] &= ~(1 << bit_idx);
        bit_idx++;
    }
    // 将空闲块位图写入硬盘
    ide_write(hhd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

    /***************************************
     *         3 初始化inode位图并写入        *
     ***************************************/
     // 先清空缓冲区
    memset(buf, 0, buf_size);
    buf[0] |= 0x1;  //第0个inode留给根目录
    // inode_table固定有4096个，正好是一个扇区，直接写入就好
    ide_write(hhd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);


    /***************************************
     *         4 初始化inode块并写入         *
     ***************************************/
     // 先清空缓冲区
    memset(buf, 0, buf_size);
    struct inode* i = (struct inode*) buf;  // 对buf转型，方便设置inode
    i->i_size = sb.dir_entry_size * 2;  // 初始化的时候，将inode大小设置为两个目录的大小：即. 和 ..
    i->i_no = 0;
    i->i_sectors[0] = sb.data_start_lba; // 把inode的第0个数据块指向sb.data_start_lba，
                                         // 即把根目录设置到最开始的空闲块, 而其他的位由于memset清空操作，都设置为0了
    ide_write(hhd, sb.inode_table_lba, buf, sb.inode_table_sects);

    /***************************************
         5 将根目录写入sb.data_start_lba
     ***************************************/
    memset(buf, 0, buf_size);   // 先清空缓冲区
    struct dir_entry* dir_entry = (struct dir_entry*)buf;

    memcpy(dir_entry->filename, ".", 1);
    dir_entry->i_no = 0; // 根目录的inode为0
    dir_entry->f_type = FT_DIRECTORY;
    dir_entry++; // 下一个目录项：父目录 ..

    memcpy(dir_entry->filename, "..", 1);
    dir_entry->i_no = 0; // 根目录的父目录还是根目录自己
    dir_entry->f_type = FT_DIRECTORY;
    ide_write(hhd, sb.data_start_lba, buf, 1);

    printk("    root_dir_lba:0x%x\n", part->name);
    printk("%s format done\n", part->name);
    // 释放缓冲区
    sys_free(buf);
}
/**
 * 在磁盘上搜索文件系统，若没有则格式化分区创建文件系统
 */
void file_sys_init() {
    uint8_t channel_no = 0, dev_no, part_idx = 0;

    // sb_buf用来存储从硬盘上读出的超级块
    struct super_block* sb_buf = (struct super_block*) sys_malloc(SECTOR_SIZE);

    if (sb_buf == NULL) {
        // 分配内存失败
        PANIC("alloc memory failed");
    }
    printk("searching filesystem...\n");
    while(channel_no < channel_cnt) {
        dev_no = 0;
        while (dev_no < 2) {
            // 遍历主盘和从盘
            if (dev_no == 0) {
                // 跳过主盘，也就是hd60M
                dev_no++;
                continue;
            }

            // 获取硬盘
            struct disk* hd = &channels[channel_no].devices[dev_no];
            struct partition* part = hd->prim_parts;
            while (part_idx < 12) { // 4个主分区 8个逻辑分区
                // 先处理主分区，再处理逻辑分区
                if (part_idx == 4) {
                    // 开始处理逻辑分区
                    part = hd->logic_parts;
                }
                if (part->sec_cnt > 0) {
                    // 如果分区存在
                    memset(sb_buf, 0, SECTOR_SIZE);
                    // 读出超级块，校验魔数
                    if (part->name == 0x474) {
                        part_idx ++;
                        part++;
                        continue;
                    }
                    ide_read(hd, part->start_lba + 1, sb_buf, 1);
                    if (sb_buf->magic == 0x19720106) {
                        printk("%s has filesystem\n", part->name);
                    } else {
                        // TODO: 其他文件系统
                        partition_format(part); // 对该分区建立文件系统
                    }
                }
                part_idx++; // 处理下一个分区
                part++;
            }
            // 下一个磁盘
            dev_no++;
        }
        // 下一个通道
        channel_no++;
    }
    // 释放超级块缓冲区
    sys_free(sb_buf);
}
