#ifndef OS_LEARN_DIR_H
#define OS_LEARN_DIR_H
#include "global.h"
#include "fs.h"
#include "inode.h"

#define MAX_FILE_NAME_LEN 16                // 文件名最大长度

/**
 * 目录结构
 */
struct dir {
    struct inode* inode;
    uint32_t dir_pos;                       // 记录在目录内的偏移
    uint8_t dir_buf[512];                   // 目录的数据缓存
};

/**
 * 目录项结构
 */
struct dir_entry {
    char filename[MAX_FILE_NAME_LEN];       // 普通文件或目录名称
    uint32_t i_no;                          // 目录项的inode编号
    enum file_type f_type;                  // 文件类型
};

#endif //OS_LEARN_DIR_H
