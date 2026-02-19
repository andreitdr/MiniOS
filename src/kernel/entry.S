.section .text.start
.code16
.globl _start
.extern boot_info
.extern kmain

.equ BOOTINFO_MAGIC, 0x544F4F42
.equ BI_MAGIC, 0
.equ BI_LFB, 4
.equ BI_WIDTH, 8
.equ BI_HEIGHT, 10
.equ BI_PITCH, 12
.equ BI_BPP, 14
.equ BI_FONT, 16
.equ BI_BOOT_DRIVE, 20

.equ TARGET_WIDTH, 800
.equ TARGET_HEIGHT, 600
.equ TARGET_BPP, 32

.equ CODE_SEL, 0x08
.equ DATA_SEL, 0x10
.equ KERNEL_BASE, 0x20000

_start:
    cli
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0xFFFE, %sp
    sti

    movb %dl, boot_info + BI_BOOT_DRIVE
    mov $BOOTINFO_MAGIC, %eax
    mov %eax, boot_info + BI_MAGIC

    call vbe_find_mode
    call get_font_ptr
    call enable_a20
    call enter_protected_mode

.hang:
    hlt
    jmp .hang

vbe_find_mode:
    pusha
    mov %cs, %ax
    mov %ax, %es
    mov $vbe_controller_info, %di

    mov $0x32454256, %eax
    mov %eax, %es:(%di)

    mov $0x4F00, %ax
    int $0x10
    cmp $0x004F, %ax
    jne vbe_fail

    mov vbe_controller_info + 0x0E, %si
    mov vbe_controller_info + 0x10, %ax
    mov %ax, %ds

.mode_loop:
    lodsw
    cmp $0xFFFF, %ax
    je vbe_fail

    push %ds
    push %si
    mov %ax, %cx
    call vbe_check_mode
    pop %si
    pop %ds
    cmp $1, %al
    je vbe_set_mode
    jmp .mode_loop

vbe_set_mode:
    mov $0x4F02, %ax
    mov selected_mode, %bx
    or $0x4000, %bx
    int $0x10
    cmp $0x004F, %ax
    jne vbe_fail

    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    popa
    ret

vbe_check_mode:
    push %ds
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov $vbe_mode_info, %di

    mov $0x4F01, %ax
    int $0x10
    cmp $0x004F, %ax
    jne .fail

    mov vbe_mode_info + 0x00, %ax
    test $0x0001, %ax
    jz .fail
    test $0x0080, %ax
    jz .fail

    mov vbe_mode_info + 0x12, %ax
    cmp $TARGET_WIDTH, %ax
    jne .fail
    mov vbe_mode_info + 0x14, %ax
    cmp $TARGET_HEIGHT, %ax
    jne .fail

    mov vbe_mode_info + 0x19, %al
    cmp $TARGET_BPP, %al
    jne .fail
    mov vbe_mode_info + 0x1B, %al
    cmp $0x06, %al
    jne .fail

    mov vbe_mode_info + 0x12, %ax
    mov %ax, boot_info + BI_WIDTH
    mov vbe_mode_info + 0x14, %ax
    mov %ax, boot_info + BI_HEIGHT
    mov vbe_mode_info + 0x10, %ax
    mov %ax, boot_info + BI_PITCH
    mov vbe_mode_info + 0x19, %al
    mov %al, boot_info + BI_BPP

    mov vbe_mode_info + 0x28, %eax
    mov %eax, boot_info + BI_LFB
    mov %cx, selected_mode

    mov $1, %al
    pop %ds
    ret

.fail:
    xor %al, %al
    pop %ds
    ret

vbe_fail:
    mov $0x0003, %ax
    int $0x10
    mov %cs, %ax
    mov %ax, %ds
    mov $msg_vbe_fail, %si
    call rm_puts
    jmp .hang

get_font_ptr:
    pusha
    mov $0x1130, %ax
    mov $0x00, %bh
    int $0x10

    push %ds
    push %es
    mov %es, %ax
    mov %ax, %ds
    mov %bp, %si
    mov %cs, %ax
    mov %ax, %es
    mov $font_buffer, %di
    mov $2048, %cx
    rep movsb
    pop %es
    pop %ds

    mov $font_buffer, %eax
    add $KERNEL_BASE, %eax
    mov %eax, boot_info + BI_FONT
    popa
    ret

enable_a20:
    pusha
    mov $0x2401, %ax
    int $0x15
    jc .fallback
    cmp $0x00, %ah
    jne .fallback
    popa
    ret

.fallback:
    in $0x92, %al
    or $0x02, %al
    out %al, $0x92
    popa
    ret

enter_protected_mode:
    cli
    lgdt gdt_descriptor
    mov %cr0, %eax
    or $0x01, %eax
    mov %eax, %cr0
    .byte 0x66, 0xEA
    .long protected_entry + KERNEL_BASE
    .word CODE_SEL

rm_puts:
    pusha
.puts_loop:
    lodsb
    test %al, %al
    jz .puts_done
    mov $0x0E, %ah
    mov $0x00, %bh
    int $0x10
    jmp .puts_loop
.puts_done:
    popa
    ret

.code32
protected_entry:
    mov $DATA_SEL, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov $0x9F000, %esp

    lea (boot_info + KERNEL_BASE), %eax
    push %eax
    call kmain

    cli
.pm_hang:
    hlt
    jmp .pm_hang

.section .rmdata
.align 4
gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_descriptor:
    .word gdt_descriptor_end - gdt - 1
    .long gdt + KERNEL_BASE
gdt_descriptor_end:

selected_mode:
    .word 0

msg_vbe_fail:
    .ascii "VBE mode not found\r\n\0"

.align 4
vbe_controller_info:
    .space 512
vbe_mode_info:
    .space 256

.align 4
font_buffer:
    .space 2048
