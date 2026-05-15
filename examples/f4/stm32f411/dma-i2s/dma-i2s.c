/*
 * I2S2 + DMA1 — Double-buffer (ping-pong) para áudio contínuo sem CPU
 *
 * STM32F411  >  UDA1334A
 *   PB12     >  WS/LRCK  (I2S2_WS,  AF5)
 *   PB13     >  BCK/SCK  (I2S2_CK,  AF5)
 *   PB15     >  DIN      (I2S2_SD,  AF5)
 *   PC6      >  MCLK     (I2S2_MCK, AF5)  opcional
 *
 * DMA1 Stream4 Canal0 = SPI2_TX  (RM0383 tabela 27)
 *
 * O buffer circular é dividido em duas metades:
 *   HTIF > DMA concluiu a 1ª metade > main refaz a 1ª enquanto DMA toca a 2ª
 *   TCIF > DMA concluiu a 2ª metade > main refaz a 2ª enquanto DMA toca a 1ª
 *
 * Clock I2S:
 *   HSE=25 MHz, PLLI2SM=25 > VCO_in=1 MHz
 *   PLLI2SN=271, PLLI2SR=2 > I2SCLK=135.5 MHz
 *   I2SDIV=6, ODD=0, MCKOE=1 > FS = 135.5e6/3072 ≈ 44108 Hz
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define SAMPLE_RATE  44100u
#define NFRAMES      512u            /* pares L+R por metade do buffer */
#define BUF_LEN      (NFRAMES * 4u)  /* half-words totais: 2 metades × 2 canais */

static int16_t audio[BUF_LEN];

/* Sinaliza qual metade do buffer precisa ser recarregada (setado na ISR, limpo em main) */
static volatile uint8_t half_ready = 0;

/* Índice global de frames para manter fase contínua entre metades */
static uint32_t frame_idx = 0;

/* ------------------------------------------------------------------ */
static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

    /* PLLI2S para STM32F411: PLLI2SM é separado do PLLM principal —
     * escrever diretamente pois rcc_plli2s_config() não seta PLLI2SM. */
    RCC_CR &= ~RCC_CR_PLLI2SON;
    RCC_PLLI2SCFGR = (2U << 28) | (271U << 6) | 25U;
    RCC_CR |= RCC_CR_PLLI2SON;
    while (!(RCC_CR & RCC_CR_PLLI2SRDY));

    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_DMA1);
}

/* ------------------------------------------------------------------ */
static void i2s2_setup(void)
{
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    GPIO12 | GPIO13 | GPIO15);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO12 | GPIO13 | GPIO15);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO12 | GPIO13 | GPIO15);

    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
    gpio_set_af(GPIOC, GPIO_AF5, GPIO6);
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO6);

    rcc_periph_reset_pulse(RST_SPI2);

    SPI2_I2SCFGR = SPI_I2SCFGR_I2SMOD
        | (SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT << SPI_I2SCFGR_I2SCFG_LSB)
        | (SPI_I2SCFGR_I2SSTD_I2S_PHILIPS     << SPI_I2SCFGR_I2SSTD_LSB)
        | (SPI_I2SCFGR_DATLEN_16BIT            << SPI_I2SCFGR_DATLEN_LSB);

    /* FS = 135.5e6 / (256 × (2×6 + 0)) = 135.5e6 / 3072 ≈ 44108 Hz */
    SPI2_I2SPR = SPI_I2SPR_MCKOE | 6;

    /* Habilita a requisição DMA do SPI2 — I2SE fica para depois do DMA pronto */
    SPI2_CR2 |= SPI_CR2_TXDMAEN;
}

/* ------------------------------------------------------------------ */
static void dma_setup(void)
{
    dma_stream_reset(DMA1, DMA_STREAM4);

    dma_set_transfer_mode(DMA1, DMA_STREAM4, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);

    /* DMA1 Stream4 Canal0 = SPI2_TX (RM0383 tabela 27) */
    dma_channel_select(DMA1, DMA_STREAM4, DMA_SxCR_CHSEL_0);
    dma_set_priority(DMA1, DMA_STREAM4, DMA_SxCR_PL_HIGH);

    /* 16 bits em ambos os lados — cada half-word = um canal I2S (L ou R) */
    dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_16BIT);
    dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);

    /* Circular: o DMA volta ao início do buffer automaticamente */
    dma_enable_circular_mode(DMA1, DMA_STREAM4);

    dma_set_peripheral_address(DMA1, DMA_STREAM4, (uint32_t)&SPI2_DR);
    dma_set_memory_address(DMA1, DMA_STREAM4, (uint32_t)audio);
    dma_set_number_of_data(DMA1, DMA_STREAM4, BUF_LEN);

    /* HTIF + TCIF para double-buffer */
    dma_enable_half_transfer_interrupt(DMA1, DMA_STREAM4);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM4);
    nvic_enable_irq(NVIC_DMA1_STREAM4_IRQ);

    dma_enable_stream(DMA1, DMA_STREAM4);
}

/* ------------------------------------------------------------------ */
/* Preenche NFRAMES pares L+R a partir do índice global de frame `start`.
 * O sinal é um acorde de quatro notas (C4, E4, G4, C5). */
static void fill_half(int16_t *buf, uint32_t start)
{
    static const float freqs[4] = {261.62f, 329.62f, 391.99f, 523.25f};
    const float amp = 32767.0f / 4.0f;

    for (uint32_t i = 0; i < NFRAMES; i++) {
        float sum = 0.0f;
        uint32_t n = start + i;
        for (int k = 0; k < 4; k++)
            sum += sinf(2.0f * M_PI * freqs[k] * (float)n / SAMPLE_RATE);
        int16_t s = (int16_t)(sum * amp);
        buf[i * 2]     = s; /* L */
        buf[i * 2 + 1] = s; /* R */
    }
}

/* ------------------------------------------------------------------ */
void dma1_stream4_isr(void)
{
    /* DMA concluiu a 1ª metade > main pode refazê-la enquanto DMA toca a 2ª */
    if (dma_get_interrupt_flag(DMA1, DMA_STREAM4, DMA_HTIF)) {
        dma_clear_interrupt_flags(DMA1, DMA_STREAM4, DMA_HTIF);
        half_ready |= 1;
    }
    /* DMA concluiu a 2ª metade > main pode refazê-la enquanto DMA toca a 1ª */
    if (dma_get_interrupt_flag(DMA1, DMA_STREAM4, DMA_TCIF)) {
        dma_clear_interrupt_flags(DMA1, DMA_STREAM4, DMA_TCIF);
        half_ready |= 2;
    }
}

/* ------------------------------------------------------------------ */
int main(void)
{
    clock_setup();
    i2s2_setup();

    /* Pré-preenche as duas metades antes de iniciar o DMA */
    fill_half(audio,             0);
    fill_half(audio + NFRAMES*2, NFRAMES);
    frame_idx = NFRAMES * 2;

    dma_setup();

    /* Habilita I2S somente após o DMA estar armado para evitar underrun inicial */
    SPI2_I2SCFGR |= SPI_I2SCFGR_I2SE;

    while (1) {
        /* Refaz a 1ª metade enquanto DMA toca a 2ª */
        if (half_ready & 1) {
            fill_half(audio, frame_idx);
            frame_idx += NFRAMES;
            half_ready &= ~(uint8_t)1;
        }
        /* Refaz a 2ª metade enquanto DMA toca a 1ª */
        if (half_ready & 2) {
            fill_half(audio + NFRAMES*2, frame_idx);
            frame_idx += NFRAMES;
            half_ready &= ~(uint8_t)2;
        }
    }
}
