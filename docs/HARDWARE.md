# FK743M2-IIT6 board assumptions

Reference: [fcaussa/FK743M2-IIT6](https://github.com/fcaussa/FK743M2-IIT6), including CubeMX .ioc (third-party tested reference; verify against your own board revision).

| Resource | Part / pins | Stage 0 |
| --- | --- | --- |
| MCU | STM32H743IIT6, LQFP176 | RT-Thread kernel |
| HSE | 25 MHz crystal | PLL input |
| Core clock | HSE / 5 * 192 / 2 = 480 MHz | candidate configuration, requires bench verification |
| On-chip Flash | 2 MiB at 0x08000000 | firmware |
| AXI SRAM | 512 KiB at 0x24000000 | current heap and link target |
| SDRAM | W9825G6KH 32 MiB at 0xC0000000 | **not initialized yet** |
| QSPI | W25Q64JV 8 MiB | **not initialized yet** |
| LED | PH7 | heartbeat |
| UART console | USART1 PA9 TX / PA10 RX | 115200 8N1; requires external 3.3V USB-TTL adapter |
| USB OTG FS | PA11 DP? **No:** PA11 = DM, PA12 = DP | **not implemented yet** |
| SWD | PA13 SWDIO / PA14 SWCLK | rescue and debugging |

**Critical**: USART3 PD8/PD9 in the generic H743 Nucleo BSP conflicts with the FK743M2 SDRAM bus. The stage-0 overlay disables that wiring by switching to USART1.

**Do not** connect the 3.3V USART pins to a true RS-232 interface or a 5V TTL output.

Current linker script and heap cover **AXI SRAM only** (512 KiB), not all the H743's physical on-chip RAM. This is intentional until external memory and MPU handling are tested.

The stage-0 application only flashes the LED and prints a heartbeat. It does not implement USB CDC, DFU, file systems or a process manager.

For dynamic modules, mapping external SDRAM as executable memory and maintaining instruction/data caches is mandatory before executing code from it.
