#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "kernel/print.h"

#define IDT_DESC_CNT 0x30 // 目前支持的中断数
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xal

#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" \
                                           : "=g"(EFLAG_VAR))
/**
 *  中断门描述结构
 */
struct gate_desc
{
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;
    uint8_t attribute;
    uint16_t func_offset_high_word;
};

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT]; // 中断门描述符数组

char *intr_name[IDT_DESC_CNT];        // 用于保存异常的名字
intr_handler idt_table[IDT_DESC_CNT]; // 定义中断函数的数组

extern intr_handler intr_entry_table[IDT_DESC_CNT]; // kernel.asm 中定义的intr_entry_table

// 配置8259A
static void pic_init(void)
{
    // 主片初始化
    outb(PIC_M_CTRL, 0x11); // ICW1
    outb(PIC_M_DATA, 0x20); // ICW2
    outb(PIC_M_DATA, 0x04); // ICW3
    outb(PIC_M_DATA, 0x01); // ICW3
    // 从片初始化
    outb(PIC_S_CTRL, 0x11); // ICW1
    outb(PIC_S_DATA, 0x28); // ICW1
    outb(PIC_S_DATA, 0x02); // ICW1
    outb(PIC_S_DATA, 0x01); // ICW1

    // 打开主片上的IR0
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    // 只打开键盘中断和时钟中断，其他全部关闭
    outb(PIC_M_DATA, 0xfc);
    outb(PIC_S_DATA, 0xff);

    put_str("   pic_init done\n");
}

static void make_idt_desc(struct gate_desc *p_gdesc, uint8_t attr, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void idt_desc_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        // 逐个注册0-32号中断函数
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("   idt_desc_init done\n");
}

// 通用的中断处理函数， 参数是中断向量号
static void general_intr_handler(uint8_t vec_nr)
{
    if (vec_nr == 0x27 | vec_nr == 0x2f)
    {
        // 8259的伪中断，无需处理
        return;
    }

    set_cursor(0);
    int cursor_pos = 0;
    // 清空320个字符
    while (cursor_pos < 320) {
        put_char(' ');
        cursor_pos++;
    }
    set_cursor(0);
    put_str("!!!!       exception message begin       !!!!\n");
    set_cursor(88); // 第二行第8个字符开始打印
    // 打印中断信息
    put_str(intr_name[vec_nr]);
    if (vec_nr == 14) {
        // pagefault错误
        int page_fault_vaddr = 0;
        asm("movl %%cr2, %0" : "=r" (page_fault_vaddr)); // cr2存放的是page_fault的地址
        put_str("\npage fault address is: ");
        put_int(page_fault_vaddr);
    }
    put_str("!!!!       exception message end         !!!!\n");
    while (1);
//    put_str("int vector : 0x");
//    put_int(vec_nr);
//    put_char('\n');
}

// 注册中断处理函数
void register_handler(uint8_t vector_no, intr_handler function) {
    idt_table[vector_no] = function;
}

static void exception_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++)
    {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown"; // 先统一赋值为 unknown
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    intr_name[16] = "#MF 0x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

void idt_init()
{
    put_str("idt_init start\n");
    idt_desc_init(); // 初始化中断描述符
    exception_init();
    pic_init(); // 初始化8259A

    // 加载idt
    // 把idt地址变成一个48位的
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0"
                 :
                 : "m"(idt_operand));
    put_str("idt_init done\n");
}

// 关中断,并返回之前的状态
enum intr_status intr_disable()
{
    enum intr_status old_status;
    if (INTR_OFF == intr_get_status())
    {
        old_status = INTR_OFF;
    }
    else
    {
        old_status = INTR_ON;
        asm volatile("cli;"
                     :
                     :
                     : "memory");
    }
    return old_status;
}
// 开中断,并返回之前的状态
enum intr_status intr_enable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
    }
    else
    {
        old_status = INTR_OFF;
        asm volatile("sti");
    }
    return old_status;
}

// 将中断设置为status
enum intr_status intr_set_status(enum intr_status status)
{
    return status & INTR_ON ? intr_enable() : intr_disable();
}

// 获取中断状态
enum intr_status intr_get_status()
{
    uint32_t eflags = 0; // eflags用来存储eflags的值
    GET_EFLAGS(eflags);
    return EFLAGS_IF & eflags ? INTR_ON : INTR_OFF;
}

