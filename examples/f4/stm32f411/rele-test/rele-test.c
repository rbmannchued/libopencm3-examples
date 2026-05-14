#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    rcc_periph_clock_enable(RCC_GPIOB);

    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO13);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);

    bool flag = false;
    bool last_state = true; // estado anterior do botão (alto = não pressionado)

    while (1) {
        bool current_state = gpio_get(GPIOB, GPIO13);

        // detectar borda de descida (botão pressionado)
        if (!current_state && last_state) {
            flag = !flag; // inverte flag
            if (flag)
                gpio_set(GPIOB, GPIO1);
            else
                gpio_clear(GPIOB, GPIO1);
        }

        last_state = current_state;

        // debounce simples (atraso curto)
        for (volatile int i = 0; i < 100000; i++); 
    }
}
