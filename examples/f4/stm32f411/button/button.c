
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

int main(void)
{

    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);


    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO2);

    while (1) {

        if (!gpio_get(GPIOB, GPIO2)) {
            gpio_set(GPIOC, GPIO13); // LED ON
        } else {
            gpio_clear(GPIOC, GPIO13); // LED OFF
        }
    }

    return 0;
}
