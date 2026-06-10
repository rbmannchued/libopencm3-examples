/*
 * TFT SPI ILI9341 (240x320) para STM32F411
 *
 * Ligação (Black Pill / STM32F411CEU6):
 *   SCK        -> PA5  (SPI1_SCK)
 *   SDI (MOSI) -> PA7  (SPI1_MOSI)
 *   SDO (MISO) -> PA6  (não usado, pode deixar desconectado)
 *   CS         -> PA4
 *   DC         -> PA3
 *   RST        -> PA2
 *   LED        -> PB0  (backlight; HIGH = ligado)
 *   VCC        -> 3.3V
 *   GND        -> GND
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

/* ── pinos */
#define TFT_PORT  GPIOA
#define TFT_SCK   GPIO5
#define TFT_MOSI  GPIO7
#define TFT_CS    GPIO4
#define TFT_DC    GPIO3
#define TFT_RST   GPIO2

#define LED_PORT  GPIOB
#define LED_PIN   GPIO0

/* ── comandos ILI9341 */
#define ILI_SWRESET  0x01
#define ILI_SLPOUT   0x11
#define ILI_GAMMA    0x26
#define ILI_DISPON   0x29
#define ILI_CASET    0x2A
#define ILI_PASET    0x2B
#define ILI_RAMWR    0x2C
#define ILI_MADCTL   0x36
#define ILI_COLMOD   0x3A
#define ILI_FRMCTR1  0xB1
#define ILI_DFUNCTR  0xB6
#define ILI_PWCTR1   0xC0
#define ILI_PWCTR2   0xC1
#define ILI_VMCTR1   0xC5
#define ILI_VMCTR2   0xC7
#define ILI_GMCTRP1  0xE0
#define ILI_GMCTRN1  0xE1

#define TFT_W  240
#define TFT_H  320

/* ── cores RGB565 */
#define BLACK    0x0000
#define WHITE    0xFFFF
#define RED      0xF800
#define GREEN    0x07E0
#define BLUE     0x001F
#define YELLOW   0xFFE0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define ORANGE   0xFD20

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * (84000 / 5); i++)
        __asm__("nop");
}

static void spi_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_SPI1);

    /* SCK, MOSI — AF5 */
    gpio_mode_setup(TFT_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TFT_SCK | TFT_MOSI);
    gpio_set_af(TFT_PORT, GPIO_AF5, TFT_SCK | TFT_MOSI);
    gpio_set_output_options(TFT_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            TFT_SCK | TFT_MOSI);

    /* CS, DC, RST — saída */
    gpio_mode_setup(TFT_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    TFT_CS | TFT_DC | TFT_RST);
    gpio_set_output_options(TFT_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            TFT_CS | TFT_DC | TFT_RST);
    gpio_set(TFT_PORT, TFT_CS);

    /* LED backlight — saída */
    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_set_output_options(LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_PIN);

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



static void tft_cmd(uint8_t cmd)
{
    gpio_clear(TFT_PORT, TFT_DC);
    gpio_clear(TFT_PORT, TFT_CS);
    spi_send(SPI1, cmd);
    while (SPI_SR(SPI1) & SPI_SR_BSY);
    gpio_set(TFT_PORT, TFT_CS);
}

static void tft_data(uint8_t data)
{
    gpio_set(TFT_PORT, TFT_DC);
    gpio_clear(TFT_PORT, TFT_CS);
    spi_send(SPI1, data);
    while (SPI_SR(SPI1) & SPI_SR_BSY);
    gpio_set(TFT_PORT, TFT_CS);
}

static void tft_reset(void)
{
    gpio_set(TFT_PORT, TFT_RST);
    delay_ms(5);
    gpio_clear(TFT_PORT, TFT_RST);
    delay_ms(20);
    gpio_set(TFT_PORT, TFT_RST);
    delay_ms(150);
}

static void tft_init(void)
{
    tft_reset();

    tft_cmd(ILI_SWRESET);  delay_ms(120);
    tft_cmd(ILI_SLPOUT);   delay_ms(120);

    tft_cmd(ILI_PWCTR1);  tft_data(0x23);
    tft_cmd(ILI_PWCTR2);  tft_data(0x10);
    tft_cmd(ILI_VMCTR1);  tft_data(0x3E); tft_data(0x28);
    tft_cmd(ILI_VMCTR2);  tft_data(0x86);

    tft_cmd(ILI_MADCTL);  tft_data(0x48); /* portrait */
    tft_cmd(ILI_COLMOD);  tft_data(0x55); /* RGB565 */

    tft_cmd(ILI_FRMCTR1); tft_data(0x00); tft_data(0x18);

    tft_cmd(ILI_DFUNCTR); tft_data(0x08); tft_data(0x82); tft_data(0x27);

    tft_cmd(ILI_GAMMA);   tft_data(0x01);

    tft_cmd(ILI_GMCTRP1);
    tft_data(0x0F); tft_data(0x31); tft_data(0x2B); tft_data(0x0C);
    tft_data(0x0E); tft_data(0x08); tft_data(0x4E); tft_data(0xF1);
    tft_data(0x37); tft_data(0x07); tft_data(0x10); tft_data(0x03);
    tft_data(0x0E); tft_data(0x09); tft_data(0x00);

    tft_cmd(ILI_GMCTRN1);
    tft_data(0x00); tft_data(0x0E); tft_data(0x14); tft_data(0x03);
    tft_data(0x11); tft_data(0x07); tft_data(0x31); tft_data(0xC1);
    tft_data(0x48); tft_data(0x08); tft_data(0x0F); tft_data(0x0C);
    tft_data(0x31); tft_data(0x36); tft_data(0x0F);

    tft_cmd(ILI_DISPON);
    delay_ms(10);

    gpio_set(LED_PORT, LED_PIN); /* liga backlight */
}

static void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    tft_cmd(ILI_CASET);
    tft_data(x0 >> 8); tft_data(x0 & 0xFF);
    tft_data(x1 >> 8); tft_data(x1 & 0xFF);

    tft_cmd(ILI_PASET);
    tft_data(y0 >> 8); tft_data(y0 & 0xFF);
    tft_data(y1 >> 8); tft_data(y1 & 0xFF);

    tft_cmd(ILI_RAMWR);
}

static void tft_fill_rect(uint16_t x, uint16_t y,
                          uint16_t w, uint16_t h, uint16_t color)
{
    tft_set_window(x, y, x + w - 1, y + h - 1);
    gpio_set(TFT_PORT, TFT_DC);
    gpio_clear(TFT_PORT, TFT_CS);
    uint8_t hi = color >> 8, lo = color & 0xFF;
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        spi_send(SPI1, hi);
        while (!(SPI_SR(SPI1) & SPI_SR_TXE));
        spi_send(SPI1, lo);
        while (!(SPI_SR(SPI1) & SPI_SR_TXE));
    }
    while (SPI_SR(SPI1) & SPI_SR_BSY);
    gpio_set(TFT_PORT, TFT_CS);
}

static void tft_fill(uint16_t color)
{
    tft_fill_rect(0, 0, TFT_W, TFT_H, color);
}

static void tft_draw_border(uint16_t color, uint16_t t)
{
    tft_fill_rect(0,         0,         TFT_W, t,     color);
    tft_fill_rect(0,         TFT_H - t, TFT_W, t,     color);
    tft_fill_rect(0,         0,         t,     TFT_H, color);
    tft_fill_rect(TFT_W - t, 0,         t,     TFT_H, color);
}


int main(void)
{
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    spi_setup();
    tft_init();

    while (1) {
        tft_fill(BLACK);
        delay_ms(500);

        /* faixas coloridas */
        tft_fill_rect(0,   0,   TFT_W, 64, RED);
        tft_fill_rect(0,  64,   TFT_W, 64, GREEN);
        tft_fill_rect(0,  128,  TFT_W, 64, BLUE);
        tft_fill_rect(0,  192,  TFT_W, 64, YELLOW);
        tft_fill_rect(0,  256,  TFT_W, 64, CYAN);
        delay_ms(1500);

        /* tela branca com borda */
        tft_fill(WHITE);
        tft_draw_border(BLACK, 4);
        delay_ms(1000);

        /* quadrados coloridos */
        tft_fill(BLACK);
        tft_fill_rect(10,  10,  100, 100, RED);
        tft_fill_rect(130, 10,  100, 100, GREEN);
        tft_fill_rect(10,  210, 100, 100, BLUE);
        tft_fill_rect(130, 210, 100, 100, MAGENTA);
        delay_ms(1500);
    }
}
