# H7OS roadmap and acceptance criteria

## Stage 0 — baseline board boot (work in progress)

- [x] Pin board target and reproducible RT-Thread v5.3.0 upstream source
- [x] Generate BSP from official stm32h743-st-nucleo example
- [x] Apply FK743M2 oscillator, USB OTG FS and PH7 configuration
- [x] Provide host build entry point and source-level tests
- [ ] Complete actual ARM cross-compilation on a machine with downloaded HAL packages
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
