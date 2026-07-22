# MIDI usb send

Send MIDI CC through USB in GPIOA 10/11(usb-c in blackpill), to test, plug the stm32 usb to a computer and use some MIDI monitor software like midi snoop and configure it to find the "STM32 MIDI SENDER" device, you will see MIDI Control Change Messages appearing every second.

Ported from the stm32f411 example; uses `rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]` since the STM32F405 tops out at 168 MHz and most F405 "core board" clones use an 8 MHz HSE crystal. If your board has a different crystal, swap `rcc_hse_8mhz_3v3` for the matching array (`rcc_hse_12mhz_3v3`, `rcc_hse_16mhz_3v3`, `rcc_hse_25mhz_3v3`), otherwise `rcc_clock_setup_pll()` will hang forever waiting for the PLL to lock and USB will never enumerate.

Also dropped the `while (gpio_get(GPIOA, GPIO8) == 0)` "wait for Vbus" loop present in the stm32f411 version: `PA8` isn't actually the OTG_FS VBUS pin (that's `PA9`), and since `OTG_GCCFG_NOVBUSSENS` already tells the peripheral to ignore Vbus sensing, that loop just hung forever on boards where `PA8` floats low — the device would enumerate (interrupt-driven) but `main()` never reached `loop()`, so no MIDI note was ever sent.
