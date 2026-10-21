#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830UL)
#define GPIOB_MODER  (*(volatile uint32_t *)0x40020400UL)
#define GPIOB_ODR    (*(volatile uint32_t *)0x40020414UL)

#define GPIOBEN      (1U << 1)
#define LED_PIN      (1U << 0)

/* Startup initialization test */
volatile uint32_t data_test = 1234U;  /* .data */
volatile uint32_t bss_test;           /* .bss */

int main(void)
{
    /* Enable GPIOB clock */
    RCC_AHB1ENR |= GPIOBEN;

    /* Read back to allow the peripheral clock to become active */
    (void)RCC_AHB1ENR;

    /* Set PB0 to output mode: MODER0 = 01 */
    GPIOB_MODER &= ~(3U << 0);
    GPIOB_MODER |=  (1U << 0);

    /* Check initialization before blinking */
    if ((data_test != 1234U) || (bss_test != 0U))
    {
        GPIOB_ODR |= LED_PIN;

        while (1)
        {
            /* Initialization failed: LED stays on */
        }
    }

    while (1)
    {
        /* Toggle LD1 */
        GPIOB_ODR ^= LED_PIN;

        /* Simple software delay */
        for (volatile uint32_t i = 0; i < 500000U; i++)
        {
        }
    }
}