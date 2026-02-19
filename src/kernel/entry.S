.code16gcc
.section .text.start
.globl _start

_start:
    cli
    xor %ax, %ax
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0xFFFE, %sp
    sti

    call kmain

.hang:
    hlt
    jmp .hang
