#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "stdio.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;
int prog_a_pid = 0, prog_b_pid = 0;

int main(void) {
    put_str("I am kernel\n");
    init_all();

    intr_enable();
    thread_start("k_thread_a", 31, k_thread_a, "arg A");
    thread_start("k_thread_b", 31, k_thread_b, "arg B");

    while(1);
    return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {
    char* para = arg;
    void* addr = sys_malloc(33); // 分配33个内存块
    printf("  I am thread_a, the addr I got is: 0x%x\n", (int)addr);
    while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {
    char* para = arg;
    void* addr = sys_malloc(63); // 分配63个内存块
    printf("  I am thread_b, the addr I got is: 0x%x\n", (int)addr);
    while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
    char* name = "prog_a";
    printf("  I am %s, my pid:%d%c", name, getpid(), '\n');
    while(1);
}

/* 测试用户进程 */
void u_prog_b(void) {
    char* name = "prog_b";
    printf("  I am %s, my pid:%d%c", name, getpid(), '\n');
    while(1);
}
