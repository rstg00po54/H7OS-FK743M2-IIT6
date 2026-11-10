# H7OS-FK743M2-IIT6

A dedicated RT-Thread-based application-runtime project for **FK743M2-IIT6 (STM32H743IIT6)**, targeting a Linux-like workflow: upload an application, launch it, stop it, and keep the kernel alive.

**Current status: stage-0 source scaffold committed. No physical-board boot, cross-build or dynamic ELF application execution has been verified yet.** Do not assume the generated image is ready to flash.

## Stage-0 features

- **RT-Thread v5.3.0** official H743 Nucleo BSP, copied into a local generated FK743M2 BSP
- FK743M2-specific **25 MHz HSE → 480 MHz candidate PLL**, **USART1 PA9/PA10**, **PH7 LED**
- Firmware stays in **2 MiB internal Flash**; kernel heap currently uses **512 KiB AXI SRAM** only
- RT-Thread MSH console expected on **USART1 115200 8N1**, with a separate 3.3V USB-UART adapter
- GitHub Actions offline tests for the adapter script; generated upstream sources are not checked in

USB CDC, DFU Runtime, SDRAM, file system, and application loading are **future milestones**, not implemented features. See [roadmap](docs/ROADMAP.md).

## Building on Ubuntu

Requirements: `git`, `python3`, `scons`, `gcc-arm-none-eabi`, and the RT-Thread Env `pkgs` command (for BSP HAL/CMSIS dependencies).

```bash
sudo apt update
sudo apt install -y git python3 scons gcc-arm-none-eabi
git clone https://github.com/rstg00po54/H7OS-FK743M2-IIT6.git
cd H7OS-FK743M2-IIT6

# Downloads RT-Thread v5.3.0 and generates the FK743M2 board-specific BSP.
python3 scripts/prepare.py

# RT-Thread Env must provide 'pkgs' first, as documented by RT-Thread.
cd third_party/rt-thread/bsp/stm32/stm32h743-fk743m2-iit6
pkgs --update

# Back in repository root:
cd ../../../../..
bash scripts/build.sh
```

Check `third_party/rt-thread/bsp/stm32/stm32h743-fk743m2-iit6/rtthread.bin` if SCons completes. **Firmware has not yet been tested on your board.**

### Test only the scripts without downloading RT-Thread

```bash
python3 -m unittest discover -s tests -v
bash -n scripts/build.sh
```

## Hardware connection for stage 0

- 3.3V USB-UART adapter **RX** ← board **PA9 TX**
- Adapter **TX** → board **PA10 RX**, GND ↔ GND
- Serial port: **115200 8N1** (Linux often `/dev/ttyUSB0`)
- PH7 LED should blink; log should contain `H7OS FK743M2-IIT6 stage-0 boot`
- Keep SWD connected to recover from bad clock/startup configuration.

See [board notes](docs/HARDWARE.md) for memory map and pin conflicts. A 5V TTL/RS232 adapter can damage the MCU.

## Why .mo instead of arbitrary .bin?

The intended application format is RT-Thread **dlmodule** (ELF-based `.mo`), with an exported system API, RAM relocations, and thread lifecycle. Arbitrary bare-metal `.bin` images are not relocatable userspace executables. This is a shared-address-space runtime, not true MMU-backed Linux process isolation.

## Source and licensing

No upstream RT-Thread or third-party board source has been copied directly into this repository: `prepare.py` generates a workspace from a pinned upstream release and locally maintained overlays.

- [RT-Thread official repository](https://github.com/RT-Thread/rt-thread)
- [Board hardware reference](https://github.com/fcaussa/FK743M2-IIT6)
- [RT-Thread dynamic modules](https://www.rt-thread.io/document/site/programming-manual/dlmodule/dlmodule/)

The FK743M2 configuration is referenced from a third-party bring-up project and must be checked against the physical board revision.
