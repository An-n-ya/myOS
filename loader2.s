%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP	equ	LOADER_BASE_ADDR
jmp loader_start

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
times 60 dq 0

SELECTOR_CODE 		equ	(0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA 		equ	(0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO		equ	(0x0003<<3) + TI_GDT + RPL0

; 以下是 gdt 的指针，前 2 字节是 gdt 界限，后 4 字节是 gdt 起始地址
gdt_ptr		dw	GDT_LIMIT
			dd	GDT_BASE
loadermsg	db	'2 loader in real.'

;------------------------------------------------------------------------------
loader_start:
						; 打印字符串，INT 0x10中断，功能号：0x13
						; 输入：
						;	AH	子功能号	=	13H
						;	BH				=	页码
						;	BL				=	属性
						;	CX				=	字符串长度
						;	(DH, DL)		=	坐标(行，列)
						;	ES:BP			=	字符串地址
						;	AL				=	显示输出地址
						;		0 ---- 字符串只含显示字符，显示后光标位置不变
						;		1 ---- 字符串只含显示字符，显示后光标位置改变
						;		2 ---- 字符串中含显示字符和显示属性，显示后光标位置不变		
						;		3 ---- 字符串中含显示字符和显示属性，显示后光标位置改变		
						; 输出：
						; 	无

	mov	sp,	LOADER_BASE_ADDR
	mov	bp,	loadermsg
	mov	cx,	17		;	字符串长度
	mov ax,	0x1301	;	AH = 13h, AL = 01h
	mov	bx,	0x001f
	mov dx,	0x1800
	int	0x10		; 	10h 号中断
						
; ;------------------------------------------------------------------------------

;---------------- 准备进入保护模式 ------------------
; 分三步：
;	1 打开A20
;	2 加载gdt
;	3 将cr0 的 pe 位置 1

	;------------ 打开A20 -------------
		in 	al, 0x92
		or 	al, 0000_0010b
		out 0x92, al

	;------------ 加载gdt -------------
		lgdt	[gdt_ptr]

	;------------ cr0 第 0 位置 1 ----------
		mov	eax, cr0
		or 	eax, 0x00000001
		mov cr0, eax
		jmp dword SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, 	ax
	mov	es,		ax
	mov ss,		ax
	mov esp,	LOADER_STACK_TOP
	mov	ax,		SELECTOR_VIDEO
	mov	gs,		ax

	mov byte	[gs:160], 'P'

	jmp $


