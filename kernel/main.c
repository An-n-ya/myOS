#include "kernel/print.h"
#include "init.h"

int main(void)
{
    put_str("I am kernel\n");
    init_all();
    asm volatile("sti"); // 在此临时开中断, 将eflags中的IF位置1
    while (1)
        ;
}