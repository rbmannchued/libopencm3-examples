#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "FreeRTOS.h"
#include "task.h"

void led_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOC);


    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
}


void led_blink_task(void *args) {
    (void) args;
    
    while (1) {
        gpio_toggle(GPIOC, GPIO13); 
        vTaskDelay(pdMS_TO_TICKS(500));  // Espera 500ms
    }
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);


    led_setup();

    xTaskCreate(led_blink_task, "LED", 256, NULL, tskIDLE_PRIORITY - 1, NULL);


    vTaskStartScheduler();


    while (1);
}
