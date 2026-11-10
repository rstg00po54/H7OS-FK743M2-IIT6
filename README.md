# H7OS-FK743M2-IIT6

A dedicated RT-Thread-based application-runtime project for **FK743M2-IIT6 (STM32H743IIT6)**, targeting a Linux-like workflow: upload an application, launch it, stop it, and keep the kernel alive.

**Current status: USB-only stage-0 overlay prepared, not verified on the actual FK743M2.** The RT-Thread USB CDC stack is configured in source, but USB enumeration, cross-compilation, board boot and dynamic ELF application execution have not yet been tested.

## Stage-0 features

- **RT-Thread v5.3.0** official H743 Nucleo BSP, copied into a local generated FK743M2 BSP
- FK743M2-specific **25 MHz HSE → 480 MHz candidate PLL**, **USB OTG FS PA11/PA12**, **PH7 LED**
- Firmware stays in **2 MiB internal Flash**; kernel heap currently uses **512 KiB AXI SRAM** only
- RT-Thread MSH console configured for **USB CDC ACM** (device name `vcom`); one USB data cable, no TTL adapter
- GitHub Actions offline tests for the adapter script; generated upstream sources are not checked in

USB CDC code is configured but **not hardware-tested**. DFU Runtime, SDRAM, filesystem, and application loading remain **future milestones**. See [roadmap](docs/ROADMAP.md).

## Building on Ubuntu

Requirements: `git`, `python3`, `scons`, `gcc-arm-none-eabi`, and the RT-Thread Env `pkgs` command (for BSP HAL/CMSIS dependencies).

```bash
sudo apt update
sudo apt install -y git python3 scons gcc-arm-none-eabi
git clone https://github.com/rstg00po54/H7OS-FK743M2-IIT6.git
cd H7OS-FK743M2-IIT6

# Downloads RT-Thread v5.3.0 and generates the FK743M2 board-specific BSP.
python3 scripts/prepare.py

# The first time only, install RT-Thread Env (official Ubuntu installer).
cd ~
wget https://raw.githubusercontent.com/RT-Thread/env/master/install_ubuntu.sh
bash install_ubuntu.sh
source ~/.env/env.sh

# Back to H7OS; this script runs 'pkgs --update' in the BSP directory.
cd ~/H7OS-FK743M2-IIT6
bash scripts/setup_deps.sh
bash scripts/build.sh
```

If Env is already installed in a new shell, activate it with `source ~/.env/env.sh`. The source checkout remains in `third_party/` and does not need to be downloaded again. Check `third_party/rt-thread/bsp/stm32/stm32h743-fk743m2-iit6/rtthread.bin` if SCons completes. **Firmware has not yet been tested on your board.**

### Test only the scripts without downloading RT-Thread

```bash
python3 -m unittest discover -s tests -v
bash -n scripts/build.sh
```

## USB-only stage 0 test

- Use a USB **data** cable connected to the FK743M2 USB OTG FS connector, not a power-only cable.
- On Ubuntu, plug in the board and run `dmesg -w` or `ls /dev/ttyACM*`.
- If enumeration works, open `picocom -b 115200 /dev/ttyACM0` (the baud setting is nominal for CDC), press Enter and run `help` or `list_thread`.
- The app should print `H7OS FK743M2-IIT6 USB stage-0 boot` and a periodic heartbeat while PH7 blinks.
- If only `0483:df11` appears, the board is still in the **STM32 ROM DFU** bootloader, not the H7OS CDC application.
- For initial flashing/recovery, use the board BOOT0/DFU procedure with STM32CubeProgrammer or `dfu-util`; a flashing command must only be used after the MCU/Flash and target boot mode have been confirmed.

See [USB notes](docs/USB_ONLY.md). USB CDC is not yet hardware-tested and the first bad clock or USB image may require **BOOT0/ROM DFU recovery**, not a serial adapter.

## Why .mo instead of arbitrary .bin?

The intended application format is RT-Thread **dlmodule** (ELF-based `.mo`), with an exported system API, RAM relocations, and thread lifecycle. Arbitrary bare-metal `.bin` images are not relocatable userspace executables. This is a shared-address-space runtime, not true MMU-backed Linux process isolation.

## Source and licensing

No upstream RT-Thread or third-party board source has been copied directly into this repository: `prepare.py` generates a workspace from a pinned upstream release and locally maintained overlays.

- [RT-Thread official repository](https://github.com/RT-Thread/rt-thread)
- [Board hardware reference](https://github.com/fcaussa/FK743M2-IIT6)
- [RT-Thread dynamic modules](https://www.rt-thread.io/document/site/programming-manual/dlmodule/dlmodule/)

The FK743M2 configuration is referenced from a third-party bring-up project and must be checked against the physical board revision.
