# H7OS-FK743M2-IIT6

RT-Thread-based embedded application-runtime project for the **FK743M2-IIT6 / STM32H743IIT6** board.

> **Status: early bring-up.** No tested firmware or independent application runtime is available yet. The first milestone is to boot RT-Thread on the actual board and obtain a working MSH console.

## Hardware

- STM32H743IIT6 (Cortex-M7); 2 MiB on-chip Flash
- 32 MiB external SDRAM (W9825G6KH)
- 8 MiB QSPI Flash (W25Q64JV)
- LED: PH7; USART1: PA9 TX / PA10 RX; USB OTG FS: PA11/PA12
- The board's HSE is **25 MHz**, per the referenced bring-up project.

## Plan

1. RT-Thread kernel, GPIO / USART1 shell, reproducible SCons build.
2. External SDRAM initialize + memory tests + executable MPU allocation.
3. File system on SD/QSPI, file upload over USB CDC.
4. RT-Thread dlmodule application loading, execution, clean exit and unloading.
5. USB DFU runtime + recoverable firmware update.

This project uses upstream RT-Thread sources and an FK743M2-specific board overlay. **Do not flash builds before reviewing and verifying the board wiring and clock.**

## References

- [RT-Thread upstream](https://github.com/RT-Thread/rt-thread)
- [FK743M2 hardware bring-up reference](https://github.com/fcaussa/FK743M2-IIT6)
- [RT-Thread dlmodule documentation](https://www.rt-thread.io/document/site/programming-manual/dlmodule/dlmodule/)

Further setup and build files will be added to this repository.
