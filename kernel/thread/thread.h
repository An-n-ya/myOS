#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "../../lib/stdint.h"
#include "../../lib/kernel/list.h"

// 通用函数类型
typedef void thread_func(void *);

// 进程状态
enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BOLCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED,
};

// 中断栈
// 用来保存中断发生时的上下文环境
// 这个栈在页的最顶端
struct intr_stack
{
    uint32_t vec_no;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;
    void (*eip)(void);
    uint32_t eflag;
    uint32_t cs;
    void *esp;
    uint32_t ss;
};

// 线程栈
// 用来在 switch_to 时保存线程环境
// 位置不固定
struct thread_stack
{
    // 注意整个结构体的顺序
    // 沿着栈的生长方向，分别是ebp, ebx, edi, esi, eip, (这个是SysV386的ABI) 下一个是返回地址，然后是线程函数和函数的参数
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    // eip是程序计数器用到的寄存器
    void (*eip)(thread_func *func, void *func_arg);

    /******下面的内容仅供第一次调度上CPU时使用*****/
    // unused_retaddr用来充当返回地址 (是用来占位的)
    void(*unused_retaddr);
    // 下面两个参数，按栈的生长方向分别是function 和 function_arg
    thread_func *function; // kernel_trhead所调用的函数名
    void *func_arg;        // 调用时所用的参数
};

// 进程/线程的pcb，程序控制块
struct task_struct
{
    uint32_t *self_kstack;
    enum task_status status;
    char name[16];
    uint8_t priority; // 线程优先级
    uint8_t ticks;    // 每次在处理器上执行的cpu时间

    uint32_t elapsed_ticks; // 自上一次cpu运行后过去了多少cpu时间

    struct list_elem general_tag; // 线程在一般队列中节点
    struct list_elem all_list_tag;

    uint32_t *pgdir;      // 栈的边界标记
    uint32_t stack_magic; // 栈的边界标记，用于检测栈溢出
};

struct task_struct *thread_start(char *name, int prio, thread_func function, void *function_arg);

#endif