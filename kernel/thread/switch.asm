[bits 32]
section .text
global switch_to
switch_to:
    push    esi
    push    edi
    push    ebx
    push    ebp
    mov     eax,    [esp + 20]      ; 得到栈中的参数cur
    mov     [eax],  esp             ; 保存栈顶指针esp

    ;------------------------  上面是备份当前线程的环境，下面是恢复下一个线程的环境   ------------------------

    mov     eax,    [esp + 24]      ; 得到栈中的参数next
    mov     esp,    [eax]           ; pcb的第一个成员self_kstack
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    ret
