#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BSP="$ROOT/third_party/rt-thread/bsp/stm32/stm32h743-fk743m2-iit6"
if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  echo "Missing arm-none-eabi-gcc: sudo apt install gcc-arm-none-eabi" >&2
  exit 1
fi
if ! command -v scons >/dev/null 2>&1; then
  echo "Missing scons: sudo apt install scons" >&2
  exit 1
fi
if [[ ! -f "$BSP/SConstruct" ]]; then
  echo "No generated BSP. Run: python3 scripts/prepare.py" >&2
  exit 1
fi
export RTT_EXEC_PATH="$(dirname "$(command -v arm-none-eabi-gcc)")"
cd "$BSP"
if [[ ! -d packages/CMSIS-Core-latest || ! -d packages/stm32h7_cmsis_driver-latest || ! -d packages/stm32h7_hal_driver-latest ]]; then
  echo "Missing RT-Thread BSP HAL/CMSIS packages."
  echo "Install the RT-Thread Env tools; in this BSP run: pkgs --update"
  echo "See README.md for details."
  exit 1
fi
scons -j"$(nproc)" "$@"
echo
echo "Build outputs: $BSP/rt-thread.elf and $BSP/rtthread.bin"
echo "Board execution NOT yet validated. Flash over SWD only after checking wiring."
