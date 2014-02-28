


.globl _start
_start:

    mov $0xABCD,r5
    mov $0xD000,sp
    mov $0x1100,-(sp)
    mov $0x0022,-(sp)
    jsr pc,_fun1
    mov r0,$0xF000
    br .
    halt
