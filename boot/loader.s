%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP	equ	LOADER_BASE_ADDR

; 构建 gdt 及其内部的描述符
GDT_BASE:			dd	0x00000000
					dd	0x00000000
CODE_DESC:			dd	0x0000FFFF
					dd	DESC_CODE_HIGH4
DATA_STACK_DESC:	dd	0x0000FFFF
					dd	DESC_DATA_HIGH4

VIDEO_DESC:			dd	0x80000007
					dd	DESC_VIDEO_HIGH4

GDT_SIZE			equ	$ - GDT_BASE
GDT_LIMIT			equ	GDT_SIZE -1
times 120 dd 0

SELECTOR_CODE 		equ	(0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA 		equ	(0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO		equ	(0x0003<<3) + TI_GDT + RPL0

; 内存大小，单位字节，此处的内存地址是0xb00
total_memory_bytes dd 0

gdt_ptr dw GDT_LIMIT
        dd GDT_BASE

ards_buf times 244 db 0
ards_nr dw 0

loader_start: 

    xor ebx, ebx
    mov edx, 0x534d4150
    mov di, ards_buf

.e820_mem_get_loop:
    mov eax, 0x0000e820
    mov ecx, 20
    int 0x15
    
    jc .e820_mem_get_failed
    
    add di, cx
    inc word [ards_nr]
    cmp ebx, 0
    jnz .e820_mem_get_loop

    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor edx, edx

.find_max_mem_area:
    mov eax, [ebx]
    add eax, [ebx + 8]
    add ebx, 20
    cmp edx, eax
    jge .next_ards
    mov edx, eax

.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

.e820_mem_get_failed:
    mov byte [gs:0], 'f'
    mov byte [gs:2], 'a'
    mov byte [gs:4], 'i'
    mov byte [gs:6], 'l'
    mov byte [gs:8], 'e'
    mov byte [gs:10], 'd'
    ; 内存检测失败，不再继续向下执行
    jmp $

.mem_get_ok:
    mov [total_memory_bytes], edx

    ; 开始进入保护模式
    ; 打开A20地址线
    in al, 0x92
    or al, 00000010B
    out 0x92, al

    ; 加载gdt
    lgdt [gdt_ptr]

    ; cr0第0位置1
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    ; 刷新流水线
    jmp dword SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
	mov ax, 	SELECTOR_DATA
	mov ds, 	ax
	mov	es,		ax
	mov ss,		ax
	mov esp,	LOADER_STACK_TOP
	mov	ax,		SELECTOR_VIDEO
	mov	gs,		ax

	mov byte	[gs:160], 'P'

;------------------------------------  加载内核  -----------------------------------------
mov eax,	KERNEL_START_SECTOR		; kernel.bin所在的扇区 默认为 0x9
mov ebx,	KERNEL_BIN_BASE_ADDR	; kernel的基址
mov ecx,	200						; 读入的扇区数

call rd_disk_m_32					; 调用读取函数，从硬盘读取内核

call setup_page						; 设置分页机制

; 保存gdt
sgdt [gdt_ptr]

; 将视频段的基址 + 0xc0000000
mov ebx, [gdt_ptr + 2]  ; gdr_ptr的低2字节是GDT界限，高四字节是地址，因此这里要加2
or dword [ebx + 0x18 + 4], 0xc0000000

; 将 gdt 的基址加上 0xc0000000
add dword [gdt_ptr + 2], 0xc0000000

; 栈指针同样映射到内核地址
add esp, 0xc0000000

; 把页目录地址赋给 cr3
mov eax, PAGE_DIR_TABLE_POS
mov cr3, eax

; 打开cr0的pg位
mov eax, cr0
or eax, 0x80000000
mov cr0, eax

; 重新加载 gdt
lgdt [gdt_ptr]

jmp SELECTOR_CODE:enter_kernel
enter_kernel:
	call 	kernel_init
	mov		esp,		0xc009f000			; 栈底 更高位置为EBDA了
	jmp		KERNEL_ENTRY_POINT				; 跳转到内核基址 (0xc0001500)

jmp $



;------------------------------------------------------------
;						下面是帮助函数
;------------------------------------------------------------

;--------------------- 创建页目录及页表 ------------------------
setup_page:
   ; 把页目录占用的空间清零
   mov ecx,    4096
   mov esi,    0
   .clear_page_dir:
      mov   byte [PAGE_DIR_TABLE_POS + esi],   0
      inc   esi
      loop  .clear_page_dir

.create_pde:
   mov   eax,  PAGE_DIR_TABLE_POS
   add   eax,  0x1000                                 ;eax此时为第一个页表的位置
   mov   ebx,  eax

   or    eax,  PG_US_U | PG_RW_W | PG_P               ; 设置RW P US为1，表示所有特权级都可以访问
   mov   [PAGE_DIR_TABLE_POS + 0x0],   eax            ; 指向第一个目录项  让虚拟地址 0x0000_0000~0x000f_ffff四兆的空间指向物理地址的0x0000_0000~0x000f_ffff的物理地址（目的是为了段机制下的线性地址和虚拟地址一致）

   mov   [PAGE_DIR_TABLE_POS + 0xc00], eax            ; 0xc00以上的目录项属于内核空间 也就是0xc000_0000~0xffff_ffff一共1G属于内核，剩下3G属于用户, 把第768个页目录项也指向第一个目录项
   sub   eax, 0x1000
   mov   [PAGE_DIR_TABLE_POS + 4092], eax             ; 使得最后一个目录项指向目录表自己的地址

; 下面创建页表项（PTE）
mov   ecx,  256
mov   esi,  0
mov   edx,  PG_US_U | PG_RW_W | PG_P
.create_pte:
   mov   [ebx+esi*4],   edx                           ; 此时的 ebx 是 0x0010_1000 即为第一个页表的地址
   add   edx,           4096                          ; 每次加 4KB
   inc   esi
   loop  .create_pte

; 创建内核其他页表的PDE
mov   eax,  PAGE_DIR_TABLE_POS
add   eax,  0x2000                                    ; 此时 eax 是第二个页表的位置
or    eax,  PG_US_U | PG_RW_W | PG_P
mov   ebx,  PAGE_DIR_TABLE_POS
mov   ecx,  254
mov   esi,  769
.create_kernel_pde:
   mov   [ebx+esi*4],   eax
   inc   esi
   add   eax,           0x1000
   loop  .create_kernel_pde
   ret


; ---------------------------- 保护模式下的硬盘读取函数 ----------------------------------
rd_disk_m_32:
	mov esi, eax		; 备份eax
	mov di, cx			; 备份cx

; 读写硬盘:
; 第一步：设置要读取的扇区数
	mov dx, 0x1f2 		; 往dx存放端口号
	mov al, cl
	out dx, al			; 读取的扇区数

	mov eax, esi		; 恢复ax

; 第二步：将LBA地址存入0x1f3 ~ 0x1f6  这四个端口一共28位，决定了lba地址
	
	; LBA 地址 7-0 位写入端口 0x1f3
	mov dx, 0x1f3
	out dx, al

	; LBA 地址 15-8 位写入端口 0x1f4
	mov cl, 8
	shr eax, cl
	mov dx, 0x1f4
	out dx, al

	; LBA 地址 23-16 位写入端口 0x1f5
	shr eax, cl
	mov dx, 0x1f5
	out dx, al
	
	; LBA 地址 27-24 位写入端口 0x1f6 最后四位在al
	shr eax, cl
	and al, 0x0f
	or al, 0xe0			; 设置 7 - 4 位为1110，表示lba模式
	mov dx, 0x1f6
	out dx, al


; 第三步：向0x1f7端口写入读命令，0x20
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al 			; 0x20是读扇区的命令

; 第四步：检测硬盘状态
.not_ready:
	; 写时表示写入命令字，读时表示读入硬盘状态
	nop 				; 空操作 相当于sleep
	in al,dx
	and al, 0x88		; 第 4 位为 1 表示硬盘控制器已经准备好数据传输了
						; 第 7 位为 1 表示硬盘忙
	cmp al, 0x08		; 如果硬盘忙，则继续等
	jnz .not_ready

; 第五步：从0x1f0端口读数据
	mov ax, di
	mov dx, 256
	mul dx
	mov cx, ax			; di为要读取的扇区数，一个扇区有512字节，每次读入一个字（两字节），共需要di*512/2次，所以di要乘以256

	mov dx, 0x1f0

.go_on_read:
	in ax, dx
	mov [ds:ebx], ax	; 这里和16位的不同
	add ebx, 2 			; 每读入2个字节，bx所指向的地址便+2
	loop .go_on_read
	ret

   

;--------------------- 将 kernel.bin 中的segment拷贝到编译的地址 -----------------------
; kernel.bin是elf格式的，需要按照elf的格式解析这个文件，并进行拷贝
kernel_init:
	xor	eax, 	eax
	xor	ebx,	ebx
	xor	ecx,	ecx
	xor	edx,	edx

	mov	dx,		[KERNEL_BIN_BASE_ADDR + 42]			; 42位置的属性是 e_phentsize, 表示 header 的大小
	mov	ebx,	[KERNEL_BIN_BASE_ADDR + 28]			; e_phoff 表示第一个header的偏移量

	add	ebx,	KERNEL_BIN_BASE_ADDR
	mov	cx,		[KERNEL_BIN_BASE_ADDR + 44]			; e_phnum 表示有几个 header, 用作循环次数

	.each_segment:
		cmp	byte	[ebx + 0],	PT_NULL				; 如果p_type为PT_NULL(0)，说明此segment未使用，直接跳过
		je	.PTNULL

		push 	dword	[ebx + 16]					; 压入参数, 16字节的偏移地址是p_filesz, 作为mym_cpy的第三个参数

		mov	eax,	[ebx + 4]						; 4字节位置是p_offset
		add	eax,	KERNEL_BIN_BASE_ADDR			; 加上kernel.bin的地址，此时eax为该段的物理地址

		push	eax									; 压入第二个参数
		push	dword	[ebx + 8]					; 压入第一个参数, 目的地址为 p_vaddr
		call	mem_cpy								; 调用复制函数
		add esp,	12								; 清除栈顶的三个参数

	
	.PTNULL:
		add	ebx,	edx								; edx 是header的大小，加上ebx进入下一个header
		loop	.each_segment						; 循环遍历每一个header
		ret

;------------------------ 逐字节拷贝 mem_cpy(dst, src, size) ------------------
; 输入： 三个参数（栈顶） dst, src, size
; 输出： 无
;----------------------------------------------------------------------------
mem_cpy:
	cld						; clean direction 将eflags寄存器中的方向标志位DF清零
	push	ebp				; 保存ebp
	mov	ebp,	esp
	push	ecx				; 保存ecx

	mov	edi,	[ebp + 8]	; dst
	mov	esi,	[ebp + 12]	; src
	mov	ecx,	[ebp + 16]	; size
	rep	movsb				; 逐字节拷贝  rep是重复执行的意思，执行次数为ecx

	; 恢复环境
	pop	ecx
	pop	ebp
	ret


	

