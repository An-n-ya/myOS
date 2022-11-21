#include "thread.h"
#include "../../lib/stdint.h"
#include "../../lib/string.h"
#include "../global.h"
#include "../memory.h"
#include "../interrupt.h"
#include "../debug.h"

#define PG_SIZE 4096

struct task_struct *main_thread;        // 主线程PCB
struct list thread_ready_list;          // 就绪队列 （只存储准备运行的队列，即status为TASK_READY的任务）
struct list thread_all_list;            // 所有任务队列 (存储包括了main线程的所有线程，包括就绪的，运行的，就绪的等等）
static struct list_elem* thread_tag;    // 用于保存队列中的线程节点

// 切换线程
extern void switch_to(struct task_struct *cur, struct task_struct *next);

// 获取当前线程的pcb
struct task_struct* running_thread() {
    uint32_t esp;
    asm("mov %%esp, %0" : "=g" (esp));
    return (struct task_struct*) (esp & 0xfffff000);
}


// 由 kernel_thread 执行线程函数
static void kernel_thread(thread_func *function, void *func_arg)
{
    // 先开中断，避免后面的时钟中断被屏蔽
    intr_enable();
    // 执行函数
    function(func_arg);
}

// 初始化kstack_stack
static void thread_create(struct task_struct *pthread, thread_func function, void *function_arg)
{
    // 预留中断使用的空间
    pthread->self_kstack -= sizeof(struct intr_stack);

    // 预留线程空间
    pthread->self_kstack -= sizeof(struct thread_stack);

    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;
    // 设置eip
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = function_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化线程基本信息
static void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    // 设置name
    strcpy(pthread->name, name);

    if (pthread == main_thread) {
        // main线程是一直运行的
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }

    // 设置优先级
    pthread->priority = prio;
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    pthread->stack_magic = 0x19720518; // 自定义魔数
}

struct task_struct *thread_start(char *name, int prio, thread_func function, void *function_arg)
{
    struct task_struct *thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, function_arg);

    // 确保之前不在就绪队列中
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    // 加入就绪队列
    list_append(&thread_ready_list, &thread->general_tag);
    // 确保线程不在全部队列中
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    // 加入全部队列
    list_append(&thread_all_list, &thread->all_list_tag);


//    // ret指令会把栈顶数据作为返回地址送上处理器的eip寄存器
//    asm volatile("movl %0, %% esp; pop %% ebp; pop %% ebp; pop %% edi; pop %% esi; ret"
//                 :
//                 : "g"(thread->self_kstack)
//                 : "memory");
    return thread;
}

static void make_main_thread(void) {
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}