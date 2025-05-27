#include  <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>


#define SER_PIN GPIO12
#define RCLK_PIN GPIO13
#define RCLR_PIN GPIO14

void shift_reg_write(uint8_t data) {
    for (int i = 7; i >= 0; i--) {
        // Define SER
        if ((data >> i) & 0x01)
            gpio_set(GPIOB, SER_PIN);
        else
            gpio_clear(GPIOB, SER_PIN);

        // Pulse on clock (SRCLK)
        gpio_clear(GPIOB, RCLR_PIN);
        gpio_set(GPIOB, RCLR_PIN);
    }

    // Pulse on  latch (RCLK)
    gpio_clear(GPIOB, RCLK_PIN);
    gpio_set(GPIOB, RCLK_PIN);
}


int main(){
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SER_PIN | RCLR_PIN | RCLK_PIN);
    shift_reg_write(0b10100111);
    while(1){
    }

    
}
