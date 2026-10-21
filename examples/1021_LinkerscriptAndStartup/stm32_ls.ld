/* Firmware entry point */
ENTRY(Reset_Handler)

/* Available memory */
MEMORY
{
	FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    	SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 192K
}

/* Initial stack pointer */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

/* Heap and stack size symbols */
__max_heap_size = 0x200;   	 /* 512 bytes */
__max_stack_size = 0x400;	/* 1024 bytes */

/* Output sections */
SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        *(.isr_vector_tbl)
        *(.text)
        *(.rodata)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT> FLASH

_sidata = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM
}

