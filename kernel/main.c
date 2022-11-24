#include "kernel/print.h"
#include "init.h"
#include "./thread/thread.h"
#include "interrupt.h"
#include "../device/console.h"

void k_thread_a(void *);
void k_thread_b(void *);


int main(void)
{
    put_str("I am kernel\n");
    init_all();
    // mem_init();
    // void *addr = get_kernel_pages(3);
    // put_str("\n get_kernel_page start vaddr is ");
    // put_int((uint32_t)addr);
    // put_str("\n");

    // asm volatile("sti"); // 在此临时开中断, 将eflags中的IF位置1
    // ASSERT(1 == 2);

    // 优先级设置为31
//    thread_start("k_thread_a", 31, k_thread_a, "argA ");
//    thread_start("k_thread_b", 8, k_thread_b, "argB ");


    intr_enable();
    while (1) {
//        console_put_str("Main ");
    };
    return 0;
}

void k_thread_a(void *arg)
{
    char *para = arg;
    while (1)
    {
        console_put_str(para);
    }
}

void k_thread_b(void *arg)
{
    char *para = arg;
    while (1)
    {
        console_put_str(para);
    }
}
