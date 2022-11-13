TI_GDT          equ 0
RPL0            equ 0
SELECTOR_VIDEO  equ (0x0003<<3) + TI_GDT + RPL0


[bits 32]
section .text
;---------------------- put_char -------------------
; 把栈中的1个字符写入光标所在处
;---------------------------------------------------
global put_char
put_char:
    pushad                      ;备份所有32位的寄存器
    mov     ax, SELECTOR_VIDEO
    mov     gs, ax              ; 把视频段放入gs段寄存器

    ; 获取光标位置
    ; 先获取高8位
    mov     dx, 0x03d4
    mov     al, 0x0e
    out     dx, al
    mov     dx, 0x03d5
    in      al, dx
    mov     ah, al
    ; 再获取低8位
    mov     dx, 0x03d4
    mov     al, 0x0f
    out     dx, al
    mov     dx, 0x03d5
    in      al, dx

    ; 将光标存入bx
    mov     bx, ax
    mov     ecx,[esp + 36]      ; 获取栈中第一个字符（由于之前保存了寄存器，占用了32字节，因此需要+36）h

    cmp     cl, 0xd             ; 对CR和LF做特殊处理
    jz      .is_carriage_return
    cmp     cl, 0xa
    jz      .is_line_feed

    cmp     cl, 0x8             ; 对退格键做处理
    jz      .is_backspace
    jmp     .put_other

    .is_backspace:
        dec     bx
        shl     bx,             1
        mov     byte [gs:bx],   0x20
        inc     bx,
        mov     byte [gs:bx],   0x07
        shr     bx,             1
        jmp     .set_cursor

    .put_other:
        shl     bx,             1
        mov     [gs:bx],        cl
        inc     bx
        mov     byte [gs:bx],   0x07
        shr     bx,             1
        inc     bx
        cmp     bx,             2000    ;判断是否需要滚屏
        jl      .set_cursor

    .is_line_feed:
    .is_carriage_return:
        xor     dx,         dx          ; dx清零，是被除数的高16为
        mov     ax,         bx          ; ax是被除数的低16位
        mov     si,         80          ; 除数
        div     si
        sub     bx,         dx          ; 光标位置 - 光标位置除以80的余数
                                        ; 即取整, 移动到行首
    .is_carriage_return_end:
        add     bx,         80
        cmp     bx,         2000
    .is_line_feed_end:
        jl      .set_cursor

    .roll_screen:
        cld
        mov     ecx,        960         ; 一共有 2000 - 80 = 1920个字符要搬运，共2840字节，一次搬运四字节
        mov     esi,        0xc00b80a0  ; 第一行行首
        mov     edi,        0xc00b8000  ; 第零行行首
        rep     movsd

        ;;;; 将最后一行填充为空白
        mov     ebx,        3840
        mov     ecx,        80
    .cls:
        mov  word [gs:ebx], 0x0720      ; 黑底白字的空格
        add     ebx,        2
        loop    .cls
        mov     bx,         1920        ; 将光标重置为最后一行行首
    
    ; 将光标设置为bx
    .set_cursor:
        mov     dx,         0x03d4
        mov     al,         0x0e
        out     dx,         al
        mov     dx,         0x03d5
        mov     al,         bh          ; 设置bx的高8位
        out     dx,         al

        mov     dx,         0x03d4
        mov     al,         0x0f
        out     dx,         al
        mov     dx,         0x03d5
        mov     al,         bl          ; 设置bx的低8位
        out     dx,         al
    .put_char_done:
        popad
        ret

;---------------- put_str --------------------------
;put_str 通过put_char来打印以0字符结尾的字符串
;---------------------------------------------------
global put_str
put_str:
    ; 备份ebx 和 ecx两个寄存器
    push    ebx
    push    ecx
    xor     ecx,    ecx             ; 清空ecx
    mov     ebx,    [esp + 12]      ; 获取待打印的字符串地址
    .go_on:
        mov     cl,     [ebx]
        cmp     cl,     0           ; 如果遇到 \0 就结束打印
        jz      .str_over
        push    ecx                 ; 保存ecx
        call    put_char            ; 打印字符
        add     esp,    4           ; 回收栈的空间，更新栈顶
        inc     ebx                 ; 使得光标后移 
        jmp     .go_on
    .str_over:
        pop     ecx
        pop     ebx                 ; 还原ecx和ebx
        ret



