# dma-i2s

Example sending buffer of signal generated through i2s using DMA, no cpu is used to send buffer only to generate the signal.

```
 STM32F411  >  UDA1334A
 PB12     >  WS/LRCK  (I2S2_WS,  AF5)
 PB13     >  BCK/SCK  (I2S2_CK,  AF5)
 PB15     >  DIN      (I2S2_SD,  AF5)
 PC6      >  MCLK     (I2S2_MCK, AF5)  optional
 ```