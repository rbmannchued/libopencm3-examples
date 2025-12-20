#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/cm3/scb.h>

static volatile uint32_t ms = 0;

void sys_tick_handler(void)
{
    ms++;
}

void delay_ms(uint32_t delay)
{
    uint32_t start = ms;
    while ((ms - start) < delay);
}

int main(){
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(RCC_GPIOB);

    systick_set_reload(72000 - 1);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();

    
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
		  GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);
    gpio_clear(GPIOB, GPIO1);

    delay_ms(30000); //delay 1/2 min
    // button click simulation
    gpio_set(GPIOB, GPIO1);
    delay_ms(200);
    gpio_clear(GPIOB, GPIO1);
    delay_ms(10);
    
    // enter standby mode
    PWR_CR |= PWR_CR_CWUF;
    PWR_CR |= PWR_CR_PDDS;
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __asm__("wfi"); 

    
    while(1){
	__asm__("nop");
    }
    
}
