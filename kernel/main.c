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
//    process_execute(u_prog_a, "u_prog_a");
//    process_execute(u_prog_b, "u_prog_b");
//    thread_start("k_thread_a", 31, k_thread_a, "arg A");
//    thread_start("k_thread_b", 31, k_thread_b, "arg B");

    while(1);
    return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {
    char* para = arg;
    void* addr1 = malloc(256);
    void* addr2 = malloc(256);
    void* addr3 = malloc(256);
    printf("  thread_a malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

    int cpu_delay = 10000;  // 让系统等会儿，再释放内存
    while (cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
    printf("  thread_a free done\n");
    while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {
    char* para = arg;
    void* addr1 = malloc(256);
    void* addr2 = malloc(256);
    void* addr3 = malloc(256);
    printf("  thread_b malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

    int cpu_delay = 10000;  // 让系统等会儿，再释放内存
    while (cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
    printf("  thread_b free done\n");
    while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
    void* addr1 = malloc(256);
    void* addr2 = malloc(256);
    void* addr3 = malloc(256);
    printf("  prog_a malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

    int cpu_delay = 10000;  // 让系统等会儿，再释放内存
    while (cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
    printf("  prog_a free done\n");
    while(1);
}

/* 测试用户进程 */
void u_prog_b(void) {
    void* addr1 = malloc(256);
    void* addr2 = malloc(256);
    void* addr3 = malloc(256);
    printf("  prog_b malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

    int cpu_delay = 10000;  // 让系统等会儿，再释放内存
    while (cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
    printf("  prog_b free done\n");
    while(1);
}
