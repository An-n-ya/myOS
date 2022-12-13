#ifndef OS_LEARN_FS_H
#define OS_LEARN_FS_H

#define MAX_FILES_PER_PART 4096     // 每个分区所支持的最大文件数
#define BITS_PER_SECTOR 4096        // 每扇区的位数
#define SECTOR_SIZE 512             // 扇区字节大小
#define BLOCK_SIZE SECTOR_SIZE      // 块字节大小

/**
 * 文件类型
 */
enum file_type {
    FT_UNKNOWN,     // 不支持的文件类型
    FT_REGULAR,     // 普通文件类型
    FT_DIRECTORY,   // 目录
};

void file_sys_init();


#endif //OS_LEARN_FS_H
