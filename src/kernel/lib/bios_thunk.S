.section .text
.code32
.globl bios_get_time_bcd_raw
.extern bios_time_bcd

.equ RM_SEG, 0x2000
.equ CODE_SEL, 0x08
.equ DATA_SEL, 0x10

bios_get_time_bcd_raw:
    cli
    pushal
    push %ds
    push %es
    push %fs
    push %gs

    mov %esp, pm_stack_ptr

    mov %cr0, %eax
    and $0xFFFFFFFE, %eax
    mov %eax, %cr0
    ljmp $RM_SEG, $rm_entry

.code16
rm_entry:
    mov $RM_SEG, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0xFFFE, %sp

    mov $0x0200, %ax
    int $0x1A

    mov %ch, bios_time_bcd
    mov %cl, bios_time_bcd + 1
    mov %dh, bios_time_bcd + 2

    mov %cr0, %eax
    or $0x01, %eax
    mov %eax, %cr0
    ljmp $CODE_SEL, $pm_return

.code32
pm_return:
    mov $DATA_SEL, %ax
    mov %ax, %ss
    mov pm_stack_ptr, %esp

    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popal
    ret

.section .bss
.align 4
pm_stack_ptr:
    .long 0

.section .data
.align 4
.globl bios_time_bcd
bios_time_bcd:
    .byte 0, 0, 0
