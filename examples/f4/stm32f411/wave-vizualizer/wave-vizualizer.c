#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/timer.h>

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"


#define FS 2000
#define NPOINTS 128
#define AMPMAX 63

volatile uint16_t buffer1[NPOINTS];
volatile uint16_t buffer2[NPOINTS];
volatile uint16_t *processing_buff = buffer2;


volatile int buffer_index = 0;

volatile bool bufferDone = false;

void i2c_setup(void) {
    /* enable clock for GPIOB and I2C1 */
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);

    

    /* configure pins PB6 (SCL) and PB7 (SDA) as Alternate Function */

    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6|GPIO7);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO6|GPIO7);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO6 | GPIO7);
    
    /* reset and config I2C */


    i2c_peripheral_disable(SSD1306_I2C_PORT);
    i2c_set_speed(SSD1306_I2C_PORT, i2c_speed_fm_400k, rcc_apb1_frequency / 1e6);

    i2c_peripheral_enable(SSD1306_I2C_PORT);
}

static void timer_init(void) {
    rcc_periph_clock_enable(RCC_TIM2);
    timer_set_prescaler(TIM2, 209);
    timer_set_period(TIM2, 199);
    timer_generate_event(TIM2, TIM_EGR_UG);
    timer_clear_flag(TIM2, TIM_EGR_UG);
    timer_set_master_mode(TIM2, TIM_CR2_MMS_UPDATE);
    timer_enable_counter(TIM2);
}


static void adc_setup(void) {
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_GPIOA);
    
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_144CYC);

    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY8);
    adc_set_resolution(ADC1, ADC_CR1_RES_12BIT);
    adc_set_single_conversion_mode(ADC1);
    uint8_t channel = 1;
    adc_set_regular_sequence(ADC1, 1, &channel);
    //dma
    adc_enable_dma(ADC1);
    adc_set_dma_continue(ADC1);
    //---
    adc_enable_external_trigger_regular(ADC1, ADC_CR2_EXTSEL_TIM2_TRGO, ADC_CR2_EXTEN_RISING_EDGE);
    adc_power_on(ADC1);

}

static void dma_setup(void) {
    rcc_periph_clock_enable(RCC_DMA2);

    dma_stream_reset(DMA2, DMA_STREAM0);
    dma_channel_select(DMA2, DMA_STREAM0, DMA_SxCR_CHSEL_0);
    dma_set_transfer_mode(DMA2, DMA_STREAM0, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_set_peripheral_address(DMA2, DMA_STREAM0, (uint32_t)&ADC1_DR);
    dma_set_memory_address(DMA2, DMA_STREAM0, (uint32_t)buffer1);   // M0AR
    dma_set_memory_address_1(DMA2, DMA_STREAM0, (uint32_t)buffer2); // M1AR
    dma_set_number_of_data(DMA2, DMA_STREAM0, NPOINTS);
    dma_set_peripheral_size(DMA2, DMA_STREAM0, DMA_SxCR_PSIZE_16BIT);
    dma_set_memory_size(DMA2, DMA_STREAM0, DMA_SxCR_MSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA2, DMA_STREAM0);
    dma_enable_double_buffer_mode(DMA2, DMA_STREAM0);
    dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM0);

    nvic_enable_irq(NVIC_DMA2_STREAM0_IRQ);
    dma_enable_stream(DMA2, DMA_STREAM0);
}

void dma2_stream0_isr(void) {
    if (dma_get_interrupt_flag(DMA2, DMA_STREAM0, DMA_TCIF)) {
	dma_clear_interrupt_flags(DMA2, DMA_STREAM0, DMA_TCIF);
	// DMA já trocou de target: o buffer que acabou é o oposto do current
	processing_buff = dma_get_target(DMA2, DMA_STREAM0) ? buffer1 :
	    buffer2;
	bufferDone = true;
    }
}
 
double normalizeToDisplay(double rawSample){
    return 63 - (rawSample * 63) / 4095;
}


int main(){
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    cm_enable_interrupts();
	
    i2c_setup();
    adc_setup();
    dma_setup();
    timer_init();

    ssd1306_Init();

    while(1){

	if (bufferDone) {
	    bufferDone = false;
	    ssd1306_Fill(Black);
	    uint8_t y_prev = normalizeToDisplay(processing_buff[0]);
	    for (uint8_t x = 1; x < NPOINTS; x++) {
		uint8_t y = normalizeToDisplay(processing_buff[x]);
		ssd1306_Line(x - 1, y_prev, x, y, White);
		y_prev = y;
	    }
	    ssd1306_UpdateScreen();
	}
	
    }
}
