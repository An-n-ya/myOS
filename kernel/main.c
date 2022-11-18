#include "kernel/print.h"
#include "init.h"
#include "debug.h"

int main(void)
{
    put_str("I am kernel\n");
    init_all();
    // asm volatile("sti"); // 在此临时开中断, 将eflags中的IF位置1
    ASSERT(1 == 2);
    while (1)
        ;
}