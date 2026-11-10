# H7OS roadmap and acceptance criteria

## Stage 0 — baseline board boot (work in progress)

- [x] Pin board target and reproducible RT-Thread v5.3.0 upstream source
- [x] Generate BSP from official stm32h743-st-nucleo example
- [x] Apply FK743M2 oscillator, USART1, and PH7 configuration
- [x] Provide host build entry point and source-level tests
- [ ] Complete actual ARM cross-compilation on a machine with downloaded HAL packages
- [ ] SWD boot on FK743M2; verify serial MSH and PH7 LED
- [ ] Check system clock and chip revision on physical board

## Stage 1 — memory

- [ ] Initialize FMC / W9825G6KH SDRAM (32 MiB)
- [ ] Destructive SDRAM pattern/address/walking-bit tests
- [ ] Reserve framebuffer region if an LCD is used
- [ ] Configure MPU for a separate executable application pool
- [ ] Verify D-Cache clean, I-Cache invalidate, and instruction execution

## Stage 2 — storage and transport

- [ ] Read/write W25Q64JV QSPI Flash, then add wear-aware filesystem
- [ ] USB CDC virtual serial with MSH
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
