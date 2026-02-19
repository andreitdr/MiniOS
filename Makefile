TOOLS_DIR=tools
LOCAL_PREFIX=$(TOOLS_DIR)/local
LOCAL_BIN=$(LOCAL_PREFIX)/bin
LOCAL_SBIN=$(LOCAL_PREFIX)/sbin

export PATH := $(LOCAL_BIN):$(LOCAL_SBIN):$(PATH)

override ASM := $(LOCAL_BIN)/nasm
override CC := $(LOCAL_BIN)/i686-elf-gcc
override LD := $(LOCAL_BIN)/i686-elf-ld
override OBJCOPY := $(LOCAL_BIN)/i686-elf-objcopy
override MKFS_FAT := $(LOCAL_SBIN)/mkfs.fat
override MCOPY := $(LOCAL_BIN)/mcopy

SRC_DIR=src
TOOLS_DIR=tools
BUILD_DIR=build

OS_NAME="WZY OS"

.PHONY: all floppy_image kernel bootloader clean always tools_fat

CFLAGS=-m32 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-builtin -Wall -Wextra -I $(SRC_DIR)/kernel/include

all: floppy_image tools_fat

#
# Floppy image
#
floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	$(MKFS_FAT) -F 12 -n $(OS_NAME) $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $(BUILD_DIR)/main_floppy.img test.txt "::test.txt"

#
# Bootloader
#
bootloader: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: always
	$(ASM) $(SRC_DIR)/bootloader/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin


KERNEL_OBJS=\
	$(BUILD_DIR)/kernel_entry.o \
	$(BUILD_DIR)/kernel_main.o \
	$(BUILD_DIR)/kernel_bootinfo.o \
	$(BUILD_DIR)/kernel_framebuffer.o \
	$(BUILD_DIR)/kernel_font8x8.o \
	$(BUILD_DIR)/kernel_mouse.o \
	$(BUILD_DIR)/kernel_time.o \
	$(BUILD_DIR)/kernel_ui.o \
	$(BUILD_DIR)/kernel_ui_widget.o \
	$(BUILD_DIR)/kernel_debug.o \
	$(BUILD_DIR)/kernel_bios_thunk.o

$(BUILD_DIR)/kernel.bin: always $(KERNEL_OBJS)
	$(LD) -m elf_i386 -T $(SRC_DIR)/kernel/linker.ld -o $(BUILD_DIR)/kernel.elf $(KERNEL_OBJS)
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel_entry.o: $(SRC_DIR)/kernel/entry.S
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_main.o: $(SRC_DIR)/kernel/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_bootinfo.o: $(SRC_DIR)/kernel/lib/bootinfo.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_framebuffer.o: $(SRC_DIR)/kernel/lib/framebuffer.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_font8x8.o: $(SRC_DIR)/kernel/lib/font8x8.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_mouse.o: $(SRC_DIR)/kernel/lib/mouse.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_time.o: $(SRC_DIR)/kernel/lib/time.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_ui.o: $(SRC_DIR)/kernel/lib/ui.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_ui_widget.o: $(SRC_DIR)/kernel/lib/ui_widget.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_debug.o: $(SRC_DIR)/kernel/lib/debug.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_bios_thunk.o: $(SRC_DIR)/kernel/lib/bios_thunk.S
	$(CC) $(CFLAGS) -c $< -o $@
	
#
# Always
#
always:
	mkdir -p $(BUILD_DIR)

#
# Clean
#
clean:
	rm -rf $(BUILD_DIR)/*