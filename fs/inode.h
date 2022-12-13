#ifndef OS_LEARN_INODE_H
#define OS_LEARN_INODE_H
#include "global.h"
#include "list.h"

struct inode {
    uint32_t i_no;              // inode 编号
    uint32_t i_size;            // inode是文件是，i_size表示文件大小，否则目录下所有文件大小之和

    uint32_t i_open_cnts;       // 记录此文件被打开的次数
    bool write_deny;            // 不能同时存在多个写文件的进程，进程写文件前需要先检查这个标识

    uint32_t i_sectors[13];     // 0~11是直接块，12是一级间接块指针
    struct list_elem inode_tag; // inode的标识(这是硬盘inode空间在内存中的表示，为防止inode重复加载，需要在速度更快的内存做标记)
};


#endif //OS_LEARN_INODE_H
