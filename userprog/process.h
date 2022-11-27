#ifndef OS_LEARN_PROCESS_H
#define OS_LEARN_PROCESS_H
#include "../kernel/thread/thread.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../kernel/debug.h"
#include "tss.h"
#include "../device/console.h"
#include "../lib/string.h"
#include "../lib/kernel/bitmap.h"

#define default_prio 31
#define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000
void process_execute(void* filename, char* name);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);

void page_dir_active(struct task_struct* p_thread);
void start_process(void* filename_);
void process_active(struct task_struct* p_thread);
#endif //OS_LEARN_PROCESS_H
