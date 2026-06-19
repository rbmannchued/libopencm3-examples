#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

int main(void) {
    /* Set STM32F446 clock to 180 MHz using the external 8MHz oscillator (HSE) */
    rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_180MHZ]);

    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);

    while (1) {
        gpio_toggle(GPIOA, GPIO1); /* Toggle LED */
        for (int i = 0; i < 45000000; i++) {
            __asm__("nop"); /* Simple delay */
        }
    }

    return 0;
}
