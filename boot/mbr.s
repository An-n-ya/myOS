%include "boot.inc" ; nasm把配置文件加载进来，该配置文件中有一些常量的定义
; 主引导程序
SECTION MBR vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00
	mov ax, 0xb800
	mov gs, ax

; 清屏
; 利用BIOS的0x06号功能，上卷全部行，清屏
	mov ax, 0x600
	mov bx, 0x700
	mov cx, 0
	mov dx, 0x184f ; 高位0x18为24 低位0x4f位79，表示一行容纳80行，共25行

	int 0x10

	mov byte [gs:0x00], '1'
	mov byte [gs:0x01], 0xA4	; A表示绿色背景闪烁，4表示前景色为红色

	mov byte [gs:0x02], ' '
	mov byte [gs:0x03], 0xA4

	mov byte [gs:0x04], 'M'
	mov byte [gs:0x05], 0xA4

	mov byte [gs:0x06], 'B'
	mov byte [gs:0x07], 0xA4

	mov byte [gs:0x08], 'R'
	mov byte [gs:0x09], 0xA4

	mov eax, LOADER_START_SECTOR  	; 起始扇区lba地址(CHS地址用起来太麻烦，用逻辑块地址）
	mov bx,  LOADER_BASE_ADDR	  	; 写入的地址
	mov cx,  4						; 待读入的扇区数
	call rd_disk_m_16				; 往下读取程序的起始部分(即cx规定的1个扇区）

	jmp LOADER_BASE_ADDR			; MBR完成使命， 将地址移动到LOADER_BASE_ADDR

; -----------------------------------------------------------------------------------------
; 功能：读取硬盘n个扇区
rd_disk_m_16:
									; 该方法使用三个参数：eax bx cx
									; eax=LBA扇区号
									; bx=将数据写入的内存地址
									; cx=读入的扇区数
; -----------------------------------------------------------------------------------------
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
	mov [bx], ax
	add bx, 2 			; 每读入2个字节，bx所指向的地址便+2
	loop .go_on_read
	ret




	times 510-($-$$) db 0

	db 0x55, 0xaa
