# tft-gmt020

2.0" SPI TFT GMT020 (ST7789 controller, 240x320) with STM32F411.

Demonstrates SPI1 initialization and drawing colored rectangles in a loop.

## Wiring

```
Display SCL  -> PA5
Display SDA  -> PA7
Display CS   -> PA4
Display DC   -> PA3
Display RST  -> PA2
Display VCC  -> 3.3V
Display GND  -> GND
```

## Build and flash

```sh
make
make flashbin
```
