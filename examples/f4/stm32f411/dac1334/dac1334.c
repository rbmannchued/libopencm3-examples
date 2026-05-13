/*
 * UDA1334A DAC via I2S2 - STM32F411 com HSE 25 MHz
 *
 * Ligações UDA1334A → STM32F411:
 *   WS/LRCK → PB12  (I2S2_WS,  AF5)
 *   BCK/SCK → PB13  (I2S2_CK,  AF5)
 *   DIN     → PB15  (I2S2_SD,  AF5)
 *   MCLK    → PC6   (I2S2_MCK, AF5)  (opcional, conecte se o módulo tiver pino MCLK)
 *   VIN     → 3.3 V
 *   GND     → GND
 *
 * Clock I2S (STM32F411 tem PLLI2SM separado — ver clock_setup):
 *   HSE=25 MHz, PLLI2SM=25 → VCO_in = 1 MHz
 *   PLLI2SN=271, PLLI2SR=2 → I2SCLK = 135.5 MHz
 *   I2SDIV=6, ODD=0         → FS = 135.5e6/(256×12) ≈ 44108 Hz
 *
 * Gera um tom de 441 Hz (seno) nos dois canais estéreo.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define SAMPLE_RATE   44100
#define TONE_HZ       441          /* 100 amostras por período exato */
#define TABLE_SIZE    (SAMPLE_RATE / TONE_HZ)  /* 100 amostras */

static int16_t sine_table[TABLE_SIZE];

/* ------------------------------------------------------------------ */
static void clock_setup(void)
{
    /* Sistema: HSE 25 MHz → 84 MHz (PLLM=25, PLLN=336, PLLP=4) */
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

    /* PLLI2S: compartilha PLLM=25, então VCO input = 1 MHz
     * N=271 → VCO = 271 MHz
     * R=2   → I2SCLK = 135.5 MHz                              */
    /* PLLI2S para STM32F411: NÃO usar rcc_plli2s_config() — ela não seta
     * PLLI2SM (bits [5:0] do PLLI2SCFGR), que no F411 é um divisor de
     * entrada SEPARADO do PLLM principal. Sem PLLI2SM correto o VCO input
     * fica 0 e o PLLI2S gera frequência errada.
     *
     * Escrita direta:
     *   PLLI2SM [5:0]  = 25  → VCO_in = 25 MHz / 25 = 1 MHz
     *   PLLI2SN [14:6] = 271 → VCO    = 271 MHz
     *   PLLI2SR [30:28]= 2   → I2SCLK = 135.5 MHz
     */
    RCC_CR &= ~RCC_CR_PLLI2SON;
    RCC_PLLI2SCFGR = (2U << 28) | (271U << 6) | 25U;
    RCC_CR |= RCC_CR_PLLI2SON;
    while (!(RCC_CR & RCC_CR_PLLI2SRDY));

    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI2);
}

/* ------------------------------------------------------------------ */
static void i2s2_setup(void)
{
    /* PB12=WS, PB13=CK, PB15=SD → AF5 (I2S2) */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO12 | GPIO13 | GPIO15);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO12 | GPIO13 | GPIO15);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO12 | GPIO13 | GPIO15);

    /* PC6=MCLK → AF5 (I2S2_MCK) — habilitar MCKOE no I2SPR se usar */
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
    gpio_set_af(GPIOC, GPIO_AF5, GPIO6);
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO6);

    rcc_periph_reset_pulse(RST_SPI2);

    /*
     * I2SCFGR:
     *   I2SMOD = 1        → modo I2S (não SPI)
     *   I2SCFG = 10       → mestre transmissor
     *   I2SSTD = 00       → padrão Philips (I2S)
     *   DATLEN = 00       → dados 16 bits
     *   CHLEN  = 0        → frame de canal 16 bits
     */
    SPI2_I2SCFGR = SPI_I2SCFGR_I2SMOD
                 | (SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT << SPI_I2SCFGR_I2SCFG_LSB)
                 | (SPI_I2SCFGR_I2SSTD_I2S_PHILIPS     << SPI_I2SCFGR_I2SSTD_LSB)
                 | (SPI_I2SCFGR_DATLEN_16BIT            << SPI_I2SCFGR_DATLEN_LSB);

    /*
     * I2SPR com MCLK habilitado (MCKOE=1), I2SCLK = 135.5 MHz:
     *   FS = 135.5e6 / (256 × (2×6 + 0)) = 135.5e6 / 3072 ≈ 44108 Hz
     *
     *   MCKOE = 1 → bit 9
     *   ODD   = 0 → bit 8
     *   I2SDIV= 6 → bits [7:0]
     */
    SPI2_I2SPR = SPI_I2SPR_MCKOE | 6;

    /* Habilita I2S */
    SPI2_I2SCFGR |= SPI_I2SCFGR_I2SE;

    /* Precarrega o primeiro sample no DR imediatamente após habilitar,
     * antes do primeiro WS edge, para evitar underrun inicial.          */
    while (!(SPI2_SR & SPI_SR_TXE));
    SPI2_DR = 0;
}

/* ------------------------------------------------------------------ */
static void generate_sine_table(void)
{
    for (int i = 0; i < TABLE_SIZE; i++) {
        float angle = 2.0f * (float)M_PI * i / TABLE_SIZE;
        sine_table[i] = (int16_t)(32767.0f * sinf(angle));
    }
}

/* Envia um par de amostras estéreo (esquerdo, direito) via I2S2 */
static void i2s2_send(int16_t left, int16_t right)
{
    while (!(SPI2_SR & SPI_SR_TXE));
    SPI2_DR = (uint16_t)left;

    while (!(SPI2_SR & SPI_SR_TXE));
    SPI2_DR = (uint16_t)right;
}

/* ------------------------------------------------------------------ */
int main(void)
{
    clock_setup();
    generate_sine_table();
    i2s2_setup();

    int idx = 0;
    while (1) {
        int16_t sample = sine_table[idx];
        i2s2_send(sample, sample);
        idx = (idx + 1 >= TABLE_SIZE) ? 0 : idx + 1;
    }
}
