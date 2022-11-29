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
    void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    void* addr7;
    printf("  thread_a start\n");
    // 如果max很大，会很耗时，得多等等才有输出
    int max = 10;
    while (max-- > 0) {
        int size = 128;
        addr1 = sys_malloc(size);
        size *= 2;
        addr2 = sys_malloc(size);
        size *= 2;
        addr3 = sys_malloc(size);
        sys_free(addr1);
        addr4 = sys_malloc(size);
        size *= 2;size *= 2;size *= 2;size *= 2;size *= 2;size *= 2;size *= 2;
        addr5 = sys_malloc(size);
        addr6 = sys_malloc(size);
        sys_free(addr5);
        size *= 2;
        addr7 = sys_malloc(size);
        sys_free(addr6);
        sys_free(addr7);
        sys_free(addr2);
        sys_free(addr3);
        sys_free(addr4);
    }
    printf("  thread_a end\n");
    while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {
    char* para = arg;
    void* addr1;
    void* addr2;
    void* addr3;
    void* addr4;
    void* addr5;
    void* addr6;
    void* addr7;
    printf("  thread_b start\n");
    int max = 10;
    while (max-- > 0) {
        int size = 16;
        addr1 = sys_malloc(size);
        sys_free(addr1);
//        size *= 2;
//        addr2 = sys_malloc(size);
//        size *= 2;
//        addr3 = sys_malloc(size);
//        sys_free(addr1);
//        addr4 = sys_malloc(size);
//        size *= 2;size *= 2;size *= 2;size *= 2;
//        addr5 = sys_malloc(size);
//        addr6 = sys_malloc(size);
//        sys_free(addr5);
//        size *= 2;
//        addr7 = sys_malloc(size);
//        sys_free(addr2);
//        sys_free(addr3);
//        sys_free(addr4);
//        sys_free(addr6);
//        sys_free(addr7);
    }
    printf("  thread_b end\n");
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
