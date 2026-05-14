# dac1334

Use a DAC (mcu1334) to generate sinewaves. With the help of a scope you will see a high and low frequency mixed signal in the LEFT Channel of the DAC  and a filtred signal in the
RIGHT Channel. A Low Pass FIR filter is applied in the RIGHT Channel.

```
 WS/LRCK > PB12  (I2S2_WS,  AF5)
 BCK/SCK > PB13  (I2S2_CK,  AF5)
 DIN     > PB15  (I2S2_SD,  AF5)
 MCLK    > PC6   (I2S2_MCK, AF5)  (opcional, conecte se o módulo tiver pino MCLK)
 VIN     > 3.3 V
 GND     > GND

```
you can edit the cutoff frequency of the FIR filter in ```filter_coeffs.c```, build the firmware running ```make``` and flashing ```make flashbin```