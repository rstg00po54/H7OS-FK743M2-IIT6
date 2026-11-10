# H7OS roadmap and acceptance criteria

## Stage 0 — baseline board boot (work in progress)

- [x] Pin board target and reproducible RT-Thread v5.3.0 upstream source
- [x] Generate BSP from official stm32h743-st-nucleo example
- [x] Apply FK743M2 oscillator, USB OTG FS and PH7 configuration
- [x] Provide host build entry point and source-level tests
- [x] Complete actual ARM cross-compilation on a machine with downloaded HAL packages
- [x] Prepare USB CDC/serial stack config and USB PCD board MSP
- [x] USB enumerates as CDC ACM on FK743M2 (2026-11-10, user verified)
- [x] ROM DFU flash H7OS on FK743M2 and verify interactive MSH via USB (2026-11-10)
- [x] Remove periodic USB console heartbeat noise; retain PH7 heartbeat and add manual `h7os` status command
- [ ] Check system clock and chip revision on physical board
- [ ] Hardware test TIM6-based software PWM on PH7 (`led pwm 25`, `led pwm 50`, `led off`, `led blink`)

## Stage 1 — memory

- [ ] Initialize FMC / W9825G6KH SDRAM (32 MiB)
- [ ] Destructive SDRAM pattern/address/walking-bit tests
- [ ] Reserve framebuffer region if an LCD is used
- [ ] Configure MPU for a separate executable application pool
- [ ] Verify D-Cache clean, I-Cache invalidate, and instruction execution

## Stage 1.5 — volatile Linux-style filesystem (not yet hardware-tested)

- [x] Integrate RT-Thread DFS v1 with nested-directory tmpfs, mounted at `/`
- [x] Create `/bin`, `/tmp`, `/home`, `/home/cc` and set cwd to `/home/cc`
- [x] Enable the existing MSH commands `ls`, `cd`, `pwd`, `mkdir`, `echo`, `cat`, `df`
- [x] Fix null-device crash in upstream `mount` listing for tmpfs
- [ ] Validate filesystem commands, memory usage and reset behavior on hardware
- [ ] Move application data to SDRAM/Flash when memory/storage drivers are ready

## Stage 2 — storage and transport

- [ ] Read/write W25Q64JV QSPI Flash, then add wear-aware filesystem
- [x] Hardware-verify USB CDC virtual serial with MSH
- [ ] Binary application upload protocol; integrity check on transfer
- [ ] Preserve ROM DFU boot path for recovery

## Stage 3 — application runtime

- [ ] Enable RT-Thread DFS + dlmodule in target configuration
- [ ] Compile a hello.mo with matching ARM hard-float ABI
- [ ] Load from /bin, invoke main(argc,argv)
- [ ] Run two modules concurrently; monitor thread ownership
- [ ] Cooperative stop, wait, free module code/data safely
- [ ] API versioning and application compatibility checks

## Stage 4 — upgrade and robustness

- [ ] DFU runtime detach or explicit jump to ROM DFU
- [ ] Reset and recover from crash without damaging boot ability
- [ ] Validate image layout and boundary checks before programming
- [ ] Automated CI cross-build, and release debug artifacts

Scope: a shared-address-space, RT-Thread application runtime—not a fully isolated Linux process model. Crashing/malicious modules may crash the kernel.

## Hardware validation — DFU and active-low LED

- [x] Invert PH7 LED logic for active-low board wiring
- [x] Integrate `dfu` command and one-shot reset-to-ROM jump
- [ ] Hardware-verify `led pwm 0`, `led pwm 25`, `led pwm 100`
- [x] Hardware-verify `dfu` disconnects CDC and enumerates 0483:df11 without BOOT0 (2026-11-10)
- [ ] Validate ROM DFU `:leave` behavior, including boot back into H7OS

- [x] Move DFU handoff from RT-Thread board init to CMSIS Reset_Handler (earlier reset stage)
- [x] Force USB software-detach before DFU software reset
- [x] Retest USB DFU enumeration after corrected reset-stage handoff (2026-11-10, user verified)
