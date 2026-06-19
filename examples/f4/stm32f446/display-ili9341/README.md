# display-ili9341

Simple ILI9341 TFT display (240×320, SPI) example for the STM32F446 using driver https://github.com/rbmannchued/ili9341-libopencm3.

## Wiring

| Module pin  | STM32F411 |
|-------------|-----------|
| VCC         | 3.3V      |
| GND         | GND       |
| SCK         | PA5       |
| SDI (MOSI)  | PA7       |
| SDO (MISO)  | PA6 (unused, can be left disconnected) |
| CS          | PA4       |
| DC          | PA3       |
| RST         | PA2       |
| LED         | PB0       |

## Build and flash

```sh
make
make flashbin
```
