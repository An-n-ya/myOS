#include "process.h"

extern void intr_exit(void);

void start_process(void* filename_) {
    void* function = filename_;
    struct task_struct* cur = running_thread();
    cur->self_kstack += sizeof(struct thread_stack);
    struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;

    proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;

    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;     // 待执行的用户程序地址
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (void*)((uint32_t) get_a_page(PF_USER, USER_STACK3_VADDR) + PAGE_SIZE);
    proc_stack->ss = SELECTOR_U_DATA;
    asm volatile("movl %0, %%esp; jmp intr_exit" : : "g"(proc_stack) : "memory"); // 假装退出中断，进入特权级3
}

/**
 * 激活页表
 */
void page_dir_active(struct task_struct* p_thread) {
    // 若为内核线程，需要重新填充页表为0x100000
    uint32_t pagedir_phy_addr = 0x100000;
    if (p_thread->pgdir != NULL) {
        // 用户态进程有自己的页目录项
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
    }
    asm volatile("movl %0, %%cr3": : "r"(pagedir_phy_addr) : "memory");
}

/**
 * 激活线程或进程的页表，更新tss中的esp0为进程的特权级0的栈
 */
void process_active(struct task_struct* p_thread){
    ASSERT(p_thread != NULL);
    page_dir_active(p_thread);      // 激活该进程或线程的页表
    if (p_thread->pgdir) {
        // 如果是用户进程，更新tss的esp0
        update_tss_esp(p_thread);
    }
}

/**
 * 创建页目录项，将当前页表表示内核空间的pde复制
 * 成功就返回目录页的虚拟地址，失败返回NULL
 */
uint32_t *create_page_dir(void) {
    uint32_t *page_dir_vaddr = get_kernel_pages(1);
    if (page_dir_vaddr == NULL) {
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }

    // 1 复制页表
    memcpy((uint32_t*) ((uint32_t) page_dir_vaddr + 0x300 * 4), (uint32_t*)(0xfffff000 + 0x300 * 4), 1024);
    // 2 更新页目录地址
    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    // 页目录地址存放在页目录的最后一项
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_R | PG_P_1;
    return page_dir_vaddr;
}

/**
 * 创建用户进城虚拟地址位图
 */
void create_user_vaddr_bitmap(struct task_struct* user_prog) {
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    // 计算位图需要多少页
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8, PAGE_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PAGE_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

/**
 * 创建用户进程
 */
void process_execute(void *filename, char* name) {
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, default_prio);
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename);
    thread->pgdir = create_page_dir();

    // 关中断 保证原子性
    enum intr_status old_status = intr_disable();

    ASSERT(!list_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!list_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    intr_set_status(old_status);
}




