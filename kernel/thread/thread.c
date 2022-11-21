#include "thread.h"
#include "../../lib/stdint.h"
#include "../../lib/string.h"
#include "../global.h"
#include "../memory.h"

#define PG_SIZE 4096

// 由 kernel_thread 执行线程函数
static void kernel_thread(thread_func *function, void *func_arg)
{
    // 执行函数
    function(func_arg);
}

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

static void init_thread(struct task_struct *pthread, char *name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    // 设置name
    strcpy(pthread->name, name);
    // 设置为正在运行
    pthread->status = TASK_RUNNING;
    // 设置优先级
    pthread->priority = prio;
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);
    pthread->stack_magic = 0x19980711; // 自定义魔数
}

struct task_struct *thread_start(char *name, int prio, thread_func function, void *function_arg)
{
    struct task_struct *thread = get_kernel_pages(1);
    init_thread(thread, name, prio);
    thread_create(thread, function, function_arg);

    // ret指令会把栈顶数据作为返回地址送上处理器的eip寄存器
    asm volatile("movl %0, %% esp; pop %% ebp; pop %% ebp; pop %% edi; pop %% esi; ret"
                 :
                 : "g"(thread->self_kstack)
                 : "memory");
    return thread;
}