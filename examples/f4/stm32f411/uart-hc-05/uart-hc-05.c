#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

void usart_setup(void) {
  // Ativar clocks
  rcc_periph_clock_enable(RCC_GPIOA);  // USART2 usa GPIOA (PA2 TX, PA3 RX)
  rcc_periph_clock_enable(RCC_USART2);

  // Configurar pinos para função alternativa (USART2)
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
  gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3); // AF7 = USART2

  // Configurar USART
  usart_set_baudrate(USART2, 115200); // HC-05 padrão é 9600 bps
  usart_set_databits(USART2, 8);
  usart_set_stopbits(USART2, USART_STOPBITS_1);
  usart_set_mode(USART2, USART_MODE_TX_RX);
  usart_set_parity(USART2, USART_PARITY_NONE);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

  // Habilitar USART
  usart_enable(USART2);
}
void usart_send_string(const char *str) {
  while (*str) {
    usart_send_blocking(USART2, *str++);
  }
}


char usart_receive_char(void) {
  return usart_recv_blocking(USART2);
}

int main(void) {
  rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
  usart_setup();

  usart_send_string("STM32 pronto para comunicar via Bluetooth!\r\n");

  while (1) {
    char c = usart_receive_char();
    usart_send_blocking(USART2, c); // eco

    for(int i = 0; i < 8000000; i++){
      __asm__("nop");
    }
  }
}
