# HM-10 HAL Library

This is a fairly simple-to-use library for HM-10 bluetooth module. At this point, it contains only peripheral-mode functionality, however adding master functions should be relatively simple.

## READ FIRST - How to use

**This library is meant to be used with RTOS and DMA. I probably won't port it to non-DMA version, because it makes the communication extremely simple to handle, compared to blocking or even interrupt methods. You could try to use this library without RTOS, but be warned - any I/O will BLOCK THE WHOLE MCU (while with RTOS, it will only block the thread running it)**.

### Requirements

An STM32 microcontroller with DMA and UART that supports idle line interrupt.
By default, this library uses HAL but it should be fairly simple to get rid of it (if you're into that kind of things), since the communication is pretty well separated from logic. Don't ask me for that thought.

The provided example is ready to use with STM32F4-series MCU (tested on STM32F411), however porting it to different MCU requires only changing the header in `hm10.hpp` file from `#include <stm32f4xx.h>` to the one that's provided for your MCU.

This example also uses a tiny printf library, which i strongly recommend (because stdlib implementation sometimes works, sometimes doesn't): https://github.com/mpaland/printf
**The library itself doesn't require it.**

