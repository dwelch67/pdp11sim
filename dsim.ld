
MEMORY
{
    ram : ORIGIN = 0x0000, LENGTH = 0xE000-0x0200
}
SECTIONS
{
    .text : { *(.text*) } > ram
    .bss : { *(.bss*) } > ram
    .data : { *(.data*) } > ram
    .rodata : { *(.rodata*) } > ram
}

