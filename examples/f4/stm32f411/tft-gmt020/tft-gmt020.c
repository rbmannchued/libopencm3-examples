/*
 * TFT SPI GMT020 (2.0", ST7789, 240x320) example for STM32F411
 *
 * Wiring (Black Pill / STM32F411CEU6):
 *   SCL (SCK)  -> PA5
 *   SDA (MOSI) -> PA7
 *   CS         -> PA4
 *   DC         -> PA3
 *   RST        -> PA2
 *   VCC        -> 3.3V
 *   GND        -> GND
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

/* ST7789 commands */
#define ST7789_SWRESET  0x01
#define ST7789_SLPOUT   0x11
#define ST7789_NORON    0x13
#define ST7789_INVON    0x21
#define ST7789_DISPON   0x29
#define ST7789_CASET    0x2A
#define ST7789_RASET    0x2B
#define ST7789_RAMWR    0x2C
#define ST7789_MADCTL   0x36
#define ST7789_COLMOD   0x3A

#define TFT_WIDTH   240
#define TFT_HEIGHT  320

/* RGB565 colors */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

static void delay_ms(uint32_t ms)
{
    /* ~84 MHz: ~84000 ciclos por ms */
    for (uint32_t i = 0; i < ms * (84000 / 5); i++)
        __asm__("nop");
}

static void spi_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_SPI1);

    /* PA5=SCK, PA7=MOSI como AF5 */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5 | GPIO7);
    gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO7);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO5 | GPIO7);

    /* PA2=RST, PA3=DC, PA4=CS como saidas */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    GPIO2 | GPIO3 | GPIO4);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO2 | GPIO3 | GPIO4);

    /* CS alto (deselect) */
    gpio_set(GPIOA, GPIO4);

    spi_init_master(SPI1,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_4,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1,
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);
    spi_enable(SPI1);
}

static void tft_write_cmd(uint8_t cmd)
{
    gpio_clear(GPIOA, GPIO3); /* DC=0: comando */
    gpio_clear(GPIOA, GPIO4); /* CS=0 */
    spi_send(SPI1, cmd);
    while (SPI_SR(SPI1) & SPI_SR_BSY);
    gpio_set(GPIOA, GPIO4);   /* CS=1 */
}

static void tft_write_data(uint8_t data)
{
    gpio_set(GPIOA, GPIO3);   /* DC=1: dado */
    gpio_clear(GPIOA, GPIO4); /* CS=0 */
    spi_send(SPI1, data);
    while (SPI_SR(SPI1) & SPI_SR_BSY);
    gpio_set(GPIOA, GPIO4);   /* CS=1 */
}

static void tft_reset(void)
{
    gpio_set(GPIOA, GPIO2);
    delay_ms(10);
    gpio_clear(GPIOA, GPIO2);
    delay_ms(50);
    gpio_set(GPIOA, GPIO2);
    delay_ms(120);
}

static void tft_init(void)
{
    tft_reset();

    tft_write_cmd(ST7789_SWRESET);
    delay_ms(150);

    tft_write_cmd(ST7789_SLPOUT);
    delay_ms(10);

    tft_write_cmd(ST7789_COLMOD);
    tft_write_data(0x55); /* 16 bits/pixel (RGB565) */

    tft_write_cmd(ST7789_MADCTL);
    tft_write_data(0x00);

    tft_write_cmd(ST7789_INVON); /* inversao de cor (necessario na maioria dos ST7789) */
    delay_ms(10);

    tft_write_cmd(ST7789_NORON);
    delay_ms(10);

    tft_write_cmd(ST7789_DISPON);
    delay_ms(10);
}

static void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    tft_write_cmd(ST7789_CASET);
    tft_write_data(x0 >> 8);
    tft_write_data(x0 & 0xFF);
    tft_write_data(x1 >> 8);
    tft_write_data(x1 & 0xFF);

    tft_write_cmd(ST7789_RASET);
    tft_write_data(y0 >> 8);
    tft_write_data(y0 & 0xFF);
    tft_write_data(y1 >> 8);
    tft_write_data(y1 & 0xFF);

    tft_write_cmd(ST7789_RAMWR);
}

static void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    tft_set_window(x, y, x + w - 1, y + h - 1);
    gpio_set(GPIOA, GPIO3);   /* DC=1 */
    gpio_clear(GPIOA, GPIO4); /* CS=0 */
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        spi_send(SPI1, color >> 8);
        while (!(SPI_SR(SPI1) & SPI_SR_TXE));
        spi_send(SPI1, color & 0xFF);
        while (!(SPI_SR(SPI1) & SPI_SR_TXE));
    }
    while (SPI_SR(SPI1) & SPI_SR_BSY);
    gpio_set(GPIOA, GPIO4);   /* CS=1 */
}

static void tft_fill(uint16_t color)
{
    tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

int main(void)
{
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    spi_setup();
    tft_init();

    while (1) {
        tft_fill(COLOR_BLACK);
        delay_ms(500);

        /* faixas coloridas */
        tft_fill_rect(0,   0, TFT_WIDTH, 80, COLOR_RED);
        tft_fill_rect(0,  80, TFT_WIDTH, 80, COLOR_GREEN);
        tft_fill_rect(0, 160, TFT_WIDTH, 80, COLOR_BLUE);
        tft_fill_rect(0, 240, TFT_WIDTH, 80, COLOR_YELLOW);
        delay_ms(1000);

        tft_fill(COLOR_WHITE);
        delay_ms(500);

        /* quadrados */
        tft_fill_rect(10,  10, 100, 100, COLOR_CYAN);
        tft_fill_rect(130, 10, 100, 100, COLOR_MAGENTA);
        tft_fill_rect(10, 210, 100, 100, COLOR_YELLOW);
        tft_fill_rect(130, 210, 100, 100, COLOR_RED);
        delay_ms(1000);
    }
}
