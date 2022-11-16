[bits 32]
%define ERROR_CODE nop         ; 错误码，这里什么都不做 nop(no operate)

%define ZERO push 0             ; 异常中CPU没压入错误码，这里手工统一压入0

extern put_str                  ; 声明外部函数

section .data
    intr_str    db  "interrupt occur!", 0xa, 0
    global      intr_entry_table
    intr_entry_table:
        %macro VECTOR 2
            section .text
                intr%1entry:
                    %2                      ; 如果有错误，压入错误码，如果没有手动压入0，对齐
                    push    intr_str
                    call    put_str
                    add     esp,    4

                    ;发送EOI
                    mov     al,     0x20    ; 中断结束命令EOI
                    out     0xa0,   al      ; 向从片发送
                    out     0x20,   al      ; 向主片发送

                    add     esp,    4
                    iret                    ; 从中断返回
            section .data
                dd  intr%1entry             ; 存储各个中断入口程序的地址，行程intr_entry_table数组

        %endmacro

    VECTOR 0x0, ZERO
    VECTOR 0x1, ZERO
    VECTOR 0x2, ZERO
    VECTOR 0x3, ZERO
    VECTOR 0x4, ZERO
    VECTOR 0x5, ZERO
    VECTOR 0x6, ZERO
    VECTOR 0x7, ZERO
    VECTOR 0x8, ZERO
    VECTOR 0x9, ZERO
    VECTOR 0xa, ZERO
    VECTOR 0xb, ZERO
    VECTOR 0xc, ZERO
    VECTOR 0xd, ZERO
    VECTOR 0xe, ZERO
    VECTOR 0xf, ZERO
    VECTOR 0x10, ZERO
    VECTOR 0x11, ZERO
    VECTOR 0x12, ZERO
    VECTOR 0x13, ZERO
    VECTOR 0x14, ZERO
    VECTOR 0x15, ZERO
    VECTOR 0x16, ZERO
    VECTOR 0x17, ZERO
    VECTOR 0x18, ZERO
    VECTOR 0x19, ZERO
    VECTOR 0x1a, ZERO
    VECTOR 0x1b, ZERO
    VECTOR 0x1c, ZERO
    VECTOR 0x1d, ZERO
    VECTOR 0x1e, ERROR_CODE
    VECTOR 0x1f, ZERO
    VECTOR 0x20, ZERO