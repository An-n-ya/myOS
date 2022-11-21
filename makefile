BUILD_DIR = out
IMG_NAME = hd60M.img
ENTRY_POINT = 0xc0001500
AS = nasm
CC = clang
LD = x86_64-elf-ld
LIB = -I lib/ -I kernel/ -I device/
ASFLAGS = -f elf
ASIB = -I include/
CFLAGS = -Wall -fno-stack-protector $(LIB) -c -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -target i386-pc-linux-elf
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/list.o

# C代码编译
$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h lib/stdint.h kernel/init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h lib/stdint.h kernel/global.h kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string.c lib/string.h lib/stdint.h kernel/global.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h lib/stdint.h kernel/global.h kernel/debug.h lib/kernel/print.h kernel/interrupt.h lib/string.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h lib/string.h lib/kernel/bitmap.h 
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/thread.o: kernel/thread/thread.c kernel/thread/thread.h kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/list.o: lib/kernel/list.c lib/kernel/list.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

# 编译loader和mbr
$(BUILD_DIR)/mbr.bin: boot/mbr.s
	$(AS) $(ASIB) $< -o $@

$(BUILD_DIR)/loader.bin: boot/loader.s
	$(AS) $(ASIB) $< -o $@

# 编译汇编
$(BUILD_DIR)/kernel.o: kernel/kernel.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o: lib/kernel/print.s
	$(AS) $(ASFLAGS) $< -o $@

# 链接
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: mk_dir hd clean all

mk_dir:
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	if [ -e "hd60M.img" ]; then rm -rf hd60M.img; fi
	bximage -q -func=create -hd=60M -imgmode=flat ${IMG_NAME}
	echo "Create image done."

hd:
	dd if=$(BUILD_DIR)/mbr.bin of=${IMG_NAME} bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/loader.bin of=${IMG_NAME} bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=${IMG_NAME} bs=512 count=200 seek=9 conv=notrunc

clean:
	rm -rf ${IMG_NAME} $(BUILD_DIR)

build: $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin

all: mk_dir build hd