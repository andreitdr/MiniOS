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

$(BUILD_DIR)/kernel.bin: always
	$(CC) -m16 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-builtin -Wall -Wextra -c $(SRC_DIR)/kernel/entry.S -o $(BUILD_DIR)/kernel_entry.o
	$(CC) -m16 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-builtin -Wall -Wextra -c $(SRC_DIR)/kernel/main.c -o $(BUILD_DIR)/kernel_main.o
	$(LD) -m elf_i386 -T $(SRC_DIR)/kernel/linker.ld -o $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel_main.o
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.bin
	
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