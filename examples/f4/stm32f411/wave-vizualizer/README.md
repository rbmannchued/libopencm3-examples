# Wave Visualizer

Simple Osciloscope. Displays an analog signal waveform on a SSD1306 128x64 OLED. Uses TIM2 to trigger ADC1 at 2 kHz, DMA2 to fill a 128-sample buffer, and I2C to update the display.

```
 PA1 > Analog input (ADC1 CH1)
 PB6 > I2C1 SCL (display)
 PB7 > I2C1 SDA (display)
```
## Build

```bash
make
make flashbin
```
