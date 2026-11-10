# FK743M2-IIT6 board assumptions

Reference: [fcaussa/FK743M2-IIT6](https://github.com/fcaussa/FK743M2-IIT6), including CubeMX .ioc (third-party tested reference; verify against your own board revision).

| Resource | Part / pins | Stage 0 |
| --- | --- | --- |
| MCU | STM32H743IIT6, LQFP176 | RT-Thread kernel |
| HSE | 25 MHz crystal | PLL input |
| Core clock | HSE / 5 * 192 / 2 = 480 MHz | candidate configuration, requires bench verification |
| On-chip Flash | 2 MiB at 0x08000000 | firmware |
| AXI SRAM | 512 KiB at 0x24000000 | current heap and link target |
| SDRAM | W9825G6KH 32 MiB at 0xC0000000 | On-demand FMC init and tests, not yet hardware verified |
| QSPI | W25Q64JV 8 MiB | **not initialized yet** |
| LED | PH7 | heartbeat |
| USB CDC Shell | USB_OTG_FS PA11 = DM, PA12 = DP | configured; untested on physical board |
| UART | USART1 PA9/PA10 | not enabled in USB-only build |
| SWD | PA13 SWDIO / PA14 SWCLK | rescue and debugging |

**Critical**: USART3 PD8/PD9 in the generic H743 Nucleo BSP conflicts with the FK743M2 SDRAM bus. The USB-only overlay disables that hardware UART and initializes OTG FS instead.

**Do not** connect the 3.3V USART pins to a true RS-232 interface or a 5V TTL output.

Current linker script and heap cover **AXI SRAM only** (512 KiB), not all the H743's physical on-chip RAM. This is intentional until external memory and MPU handling are tested.

The stage-0 application blinks the LED and attempts to provide a USB CDC MSH console. Its USB enumeration is not yet verified on FK743M2 hardware. ROM DFU remains separate; the app does not implement DFU Runtime, filesystems or a process manager.

For dynamic modules, mapping external SDRAM as executable memory and maintaining instruction/data caches is mandatory before executing code from it.
