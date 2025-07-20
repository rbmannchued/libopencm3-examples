#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/systick.h>
#include <stdio.h>

static void clock_setup(void) {
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_USART1);
}

static void usart_setup(void) {
    // Habilita GPIOA e USART1
    rcc_periph_clock_enable(RCC_USART1);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10); // PA9 = TX, PA10 = RX (AF7)

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX); // ou USART_MODE_TX_RX se quiser ler também
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);
}

static void adc_setup(void) {
    // PA4 (ADC1_IN4) e PA5 (ADC1_IN5) como analógicos
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4 | GPIO5);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_144CYC);
    adc_power_on(ADC1);
    /* adc_reset_calibration(ADC1); */
    /* adc_calibrate(ADC1); */
}

static uint16_t read_adc(uint8_t channel) {
    adc_set_regular_sequence(ADC1, 1, &channel);
    adc_start_conversion_regular(ADC1);
    while (!adc_eoc(ADC1));
    return adc_read_regular(ADC1);
}

static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 8000; ++i) {
        __asm__("nop");
    }
}

int main(void) {
    clock_setup();
    usart_setup();
    adc_setup();

    while (1) {
        uint16_t value_pa4 = read_adc(4); // ADC1_IN4 = PA4
        uint16_t value_pa5 = read_adc(5); // ADC1_IN5 = PA5

        char buf[64];
        snprintf(buf, sizeof(buf), "PA4: %u\tPA5: %u\r\n", value_pa4, value_pa5);
        for (char *c = buf; *c; c++) {
            usart_send_blocking(USART1, *c);
        }

        delay_ms(500);
    }
}
