# H7OS-FK743M2-IIT6

A dedicated RT-Thread-based application-runtime project for **FK743M2-IIT6 (STM32H743IIT6)**, targeting a Linux-like workflow: upload an application, launch it, stop it, and keep the kernel alive.

**Current status: USB-only stage-0 boot, USB CDC ACM, and MSH shell verified on the actual FK743M2-IIT6 (2026-11-10).** The initial logs confirm `list thread` and `free` work. SDRAM, QSPI, runtime DFU and dynamic ELF application execution have not yet been tested.

## Stage-0 features

- **RT-Thread v5.3.0** official H743 Nucleo BSP, copied into a local generated FK743M2 BSP
- FK743M2-specific **25 MHz HSE → 480 MHz candidate PLL**, **USB OTG FS PA11/PA12**, **PH7 LED**
- Firmware stays in **2 MiB internal Flash**; kernel heap currently uses **512 KiB AXI SRAM** only
- RT-Thread MSH console configured for **USB CDC ACM** (device name `vcom`); one USB data cable, no TTL adapter
- GitHub Actions offline tests for the adapter script; generated upstream sources are not checked in

USB CDC and MSH have been **verified on the physical board**. The PH7 LED is **active-low** (verified by board testing). Its TIM6 interrupt-based software PWM (100Hz / 1% steps) supports `led pwm 0..100`, `led on`, `led off`, `led blink`, and `led status`. The updated polarity needs a new firmware flash. A `dfu` MSH command is now provided to reboot into the STM32 ROM USB DFU bootloader using a one-shot DTCM reset flag (source-integrated, **not yet hardware-verified**). Hardware USART is intentionally excluded from the USB-only build while retaining RT-Thread's serial framework for CDC. Periodic console heartbeat logs were removed to prevent corrupting interactive command entry; PH7 still toggles and the `h7os` MSH command reports uptime ticks and LED toggle count. The Shell now shows `cc@H7OS:~$ `, matching a Linux terminal style (the username, hostname, and `~` are fixed display text until user accounts and a working-directory-aware filesystem are implemented). Runtime DFU, SDRAM, filesystem, and application loading remain **future milestones**. See [roadmap](docs/ROADMAP.md).

## Building on Ubuntu

Requirements: `git`, `python3`, `scons`, `gcc-arm-none-eabi`, and the RT-Thread Env `pkgs` command (for BSP HAL/CMSIS dependencies).

```bash
sudo apt update
sudo apt install -y git python3 scons gcc-arm-none-eabi
git clone https://github.com/rstg00po54/H7OS-FK743M2-IIT6.git
cd H7OS-FK743M2-IIT6

# Downloads RT-Thread v5.3.0 and generates the FK743M2 board-specific BSP.
python3 scripts/prepare.py

# One-time RT-Thread Env setup, using GitHub only (no Gitee login).
sudo apt install -y python3-venv
bash scripts/install_env_github.sh
source ~/.env/env.sh

bash scripts/setup_deps.sh
bash scripts/build.sh
```

If Env is already installed in a new shell, activate it with `source ~/.env/env.sh`. The helper `scripts/install_env_github.sh` specifically bypasses the upstream installer’s automatic Gitee mirror detection. It can repair remote URLs of existing valid clones but will not delete a nonempty directory. The source checkout remains in `third_party/` and does not need to be downloaded again. Check `third_party/rt-thread/bsp/stm32/stm32h743-fk743m2-iit6/rtthread.bin` if SCons completes. **The stage-0 USB CDC and Shell were tested on the board; every new build still requires validation before relying on it.**

### Test only the scripts without downloading RT-Thread

```bash
python3 -m unittest discover -s tests -v
bash -n scripts/build.sh
```

## USB-only stage 0 test

- Use a USB **data** cable connected to the FK743M2 USB OTG FS connector, not a power-only cable.
- On Ubuntu, plug in the board and run `dmesg -w` or `ls /dev/ttyACM*`.
- If enumeration works, open `picocom -b 115200 /dev/ttyACM0` (the baud setting is nominal for CDC), press Enter and run `help`, `list thread`, `ps`, `free` or `h7os`.
- The app prints `H7OS FK743M2-IIT6 USB stage-0 boot` once. The PH7 LED then blinks silently. Use `h7os` to query RTOS ticks, and `led pwm 25` to set 25% software PWM duty. `led blink` restores the 500ms blink.
- If only `0483:df11` appears, the board is still in the **STM32 ROM DFU** bootloader, not the H7OS CDC application.
- For initial flashing/recovery, use the board BOOT0/DFU procedure with STM32CubeProgrammer or `dfu-util`; a flashing command must only be used after the MCU/Flash and target boot mode have been confirmed.

See [USB notes](docs/USB_ONLY.md). Although USB CDC is now hardware-tested, a bad clock or USB image may require **BOOT0/ROM DFU recovery**, not a serial adapter.

## Why .mo instead of arbitrary .bin?

The intended application format is RT-Thread **dlmodule** (ELF-based `.mo`), with an exported system API, RAM relocations, and thread lifecycle. Arbitrary bare-metal `.bin` images are not relocatable userspace executables. This is a shared-address-space runtime, not true MMU-backed Linux process isolation.

## Source and licensing

No upstream RT-Thread or third-party board source has been copied directly into this repository: `prepare.py` generates a workspace from a pinned upstream release and locally maintained overlays.

- [RT-Thread official repository](https://github.com/RT-Thread/rt-thread)
- [Board hardware reference](https://github.com/fcaussa/FK743M2-IIT6)
- [RT-Thread dynamic modules](https://www.rt-thread.io/document/site/programming-manual/dlmodule/dlmodule/)

The FK743M2 configuration is referenced from a third-party bring-up project and must be checked against the physical board revision.
