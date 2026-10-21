#include <stdint.h>

/* Symbols defined in the linker script */
extern uint32_t _estack;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _sidata;

/* Function declarations */
void Reset_Handler(void);
void Default_Handler(void);
int main(void);

/* Core exception handlers */
void NMI_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void HardFault_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void MemManage_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void BusFault_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void UsageFault_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void SVC_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void DebugMon_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void PendSV_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void SysTick_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

/* Vector table: initial stack pointer and core exceptions */
const uint32_t vector_tbl[]
    __attribute__((section(".isr_vector_tbl"), used)) =
{
    (uint32_t)&_estack,             /*  0: Initial stack pointer */
    (uint32_t)&Reset_Handler,       /*  1: Reset */
    (uint32_t)&NMI_Handler,         /*  2: NMI */
    (uint32_t)&HardFault_Handler,   /*  3: HardFault */
    (uint32_t)&MemManage_Handler,   /*  4: MemManage */
    (uint32_t)&BusFault_Handler,    /*  5: BusFault */
    (uint32_t)&UsageFault_Handler,  /*  6: UsageFault */
    0,                             /*  7: Reserved */
    0,                             /*  8: Reserved */
    0,                             /*  9: Reserved */
    0,                             /* 10: Reserved */
    (uint32_t)&SVC_Handler,         /* 11: SVCall */
    (uint32_t)&DebugMon_Handler,    /* 12: Debug monitor */
    0,                             /* 13: Reserved */
    (uint32_t)&PendSV_Handler,      /* 14: PendSV */
    (uint32_t)&SysTick_Handler,     /* 15: SysTick */

/* Peripheral interrupts: IRQ 0 through IRQ 90 */
    [16 ... 106] = (uint32_t)&Default_Handler
};


/* Default exception/interrupt handler */
void Default_Handler(void)
{
    while (1)
    {
    }
}

/* Initialize memory and start the application */
void Reset_Handler(void)
{
    /* Copy .data initial values from Flash to SRAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* Clear .bss in SRAM */
    dst = &_sbss;

    while (dst < &_ebss)
    {
        *dst++ = 0U;
    }

    /* Start the application */
    main();

    /* Stay here if main() returns */
    while (1)
    {
    }
}