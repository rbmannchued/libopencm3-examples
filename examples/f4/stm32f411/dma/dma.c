/*
 * DMA exemplo: DMA2 Stream7 Ch4 -> USART1_TX
 *
 *   PA9  = USART1 TX (AF7) -> conecte ao RX do seu adaptador USB-Serial
 *
 * O que demonstra:
 *   1. Configuracao do DMA (modo memoria->periferico, 8-bit, incremento de memoria)
 *   2. Linkagem com USART1 via canal 4 do DMA2
 *   3. Interrupcao de Transfer Complete para saber quando o DMA terminou
 *   4. CPU livre durante a transferencia (nao usa polling de TX)
 *
 * Como verificar: abra um terminal serial em 115200 8N1. A mensagem deve
 * aparecer uma vez por segundo.
 *
 * Mapeamento DMA2 (RM0383 tabela 28):
 *   DMA2 Stream7 Canal4 = USART1_TX
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <stdbool.h>

static volatile bool transfer_done;

static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_DMA2);
}

static void usart_setup(void)
{
    /* PA9 = USART1_TX em funcao alternativa 7 */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO9);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);

    /* Habilita requisicao DMA no registrador CR3 (bit DMAT) */
    usart_enable_tx_dma(USART1);
}

static void dma_setup(void)
{
    /* Sempre faca reset do stream antes de reconfigurar */
    dma_stream_reset(DMA2, DMA_STREAM7);

    /* Direcao: memoria -> periferico */
    dma_set_transfer_mode(DMA2, DMA_STREAM7, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);

    /* Canal 4 do DMA2 Stream7 = USART1_TX (ver tabela de requisicoes do RM) */
    dma_channel_select(DMA2, DMA_STREAM7, DMA_SxCR_CHSEL_4);

    dma_set_priority(DMA2, DMA_STREAM7, DMA_SxCR_PL_HIGH);

    /* Tamanho de cada item: 8 bits tanto na memoria quanto no periferico */
    dma_set_memory_size(DMA2, DMA_STREAM7, DMA_SxCR_MSIZE_8BIT);
    dma_set_peripheral_size(DMA2, DMA_STREAM7, DMA_SxCR_PSIZE_8BIT);

    /* O ponteiro de memoria avanca a cada byte; o endereco do periferico e fixo */
    dma_enable_memory_increment_mode(DMA2, DMA_STREAM7);

    /* Periferico destino: registrador de dados do USART1 */
    dma_set_peripheral_address(DMA2, DMA_STREAM7, (uint32_t)&USART1_DR);

    /* Habilita interrupcao de Transfer Complete */
    dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM7);
    nvic_enable_irq(NVIC_DMA2_STREAM7_IRQ);
}

/* Dispara uma transferencia DMA a partir do buffer `buf` com `len` bytes */
static void dma_send(const char *buf, uint16_t len)
{
    transfer_done = false;

    /* Desabilita o stream para poder reconfigurar endereco e tamanho */
    dma_disable_stream(DMA2, DMA_STREAM7);

    dma_set_memory_address(DMA2, DMA_STREAM7, (uint32_t)buf);
    dma_set_number_of_data(DMA2, DMA_STREAM7, len);

    /* Habilitar o stream dispara a transferencia */
    dma_enable_stream(DMA2, DMA_STREAM7);
}

/* ISR do DMA2 Stream7: chamada quando o DMA termina de copiar todos os bytes */
void dma2_stream7_isr(void)
{
    if (dma_get_interrupt_flag(DMA2, DMA_STREAM7, DMA_TCIF)) {
        dma_clear_interrupt_flags(DMA2, DMA_STREAM7, DMA_TCIF);
        transfer_done = true;
    }
}

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 8400; i++)
        __asm__("nop");
}

int main(void)
{
    const char msg[] = "[DMA] transferencia concluida pelo hardware!\r\n";

    clock_setup();
    usart_setup();
    dma_setup();

    while (1) {
        /* Dispara: a CPU nao escreve nenhum byte no USART, o DMA faz tudo */
        dma_send(msg, sizeof(msg) - 1);

        /* Aguarda a ISR sinalizar conclusao */
        while (!transfer_done);

        delay_ms(1000);
    }
}
