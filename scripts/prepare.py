#!/usr/bin/env python3
"""Create an FK743M2 BSP overlay from the pinned official RT-Thread H743 BSP.

This deliberately copies the upstream BSP into an ignored directory and patches
only the board-specific essentials. It does NOT claim SDRAM / QSPI / USB are ready.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REPO = "https://github.com/RT-Thread/rt-thread.git"
TAG = "v5.3.0"
SOURCE = "stm32h743-st-nucleo"
TARGET = "stm32h743-fk743m2-iit6"
UPSTREAM = ROOT / "third_party" / "rt-thread"
BSP_ROOT = UPSTREAM / "bsp" / "stm32"
TARGET_PATH = BSP_ROOT / TARGET


def replace_exact(text: str, old: str, new: str, name: str) -> str:
    if old not in text:
        raise ValueError(f"Expected upstream text missing in {name}: {old!r}")
    return text.replace(old, new)


def patch(path: Path, pairs: list[tuple[str, str]]) -> None:
    original = path.read_text(encoding="utf-8")
    result = original
    for old, new in pairs:
        result = replace_exact(result, old, new, str(path))
    path.write_text(result, encoding="utf-8")


def checkout_upstream() -> None:
    if not UPSTREAM.exists():
        UPSTREAM.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            ["git", "clone", "--depth", "1", "--single-branch", "--branch", TAG,
             REPO, str(UPSTREAM)], check=True
        )
    if not (UPSTREAM / ".git").exists():
        raise RuntimeError(f"{UPSTREAM} exists but is not a Git checkout")
    head = subprocess.check_output(
        ["git", "-C", str(UPSTREAM), "rev-parse", "HEAD"], text=True
    ).strip()
    pinned = subprocess.check_output(
        ["git", "-C", str(UPSTREAM), "rev-list", "-n", "1", TAG], text=True
    ).strip()
    if head != pinned:
        raise RuntimeError(
            f"Existing checkout is not {TAG} (HEAD={head}). "
            "Keep a separate copy or re-clone it."
        )


def create_bsp() -> None:
    source = BSP_ROOT / SOURCE
    if not source.is_dir():
        raise RuntimeError(f"Missing source BSP: {source}")
    if TARGET_PATH.exists():
        shutil.rmtree(TARGET_PATH)
    shutil.copytree(source, TARGET_PATH)

    patch(TARGET_PATH / "Kconfig", [
        ("SOC_STM32H743ZI", "SOC_STM32H743II"),
        ("BOARD_STM32H743_NUCLEO", "BOARD_FK743M2_IIT6"),
        ("    select BOARD_SERIES_STM32_NUCLEO_144\n", ""),
    ])
    patch(TARGET_PATH / ".config", [
        ("CONFIG_SOC_STM32H743ZI=y", "CONFIG_SOC_STM32H743II=y"),
        ("CONFIG_BOARD_STM32H743_NUCLEO=y", "CONFIG_BOARD_FK743M2_IIT6=y"),
        ("CONFIG_BOARD_SERIES_STM32_NUCLEO_144=y\n", ""),
        ('CONFIG_RT_CONSOLE_DEVICE_NAME="uart3"', 'CONFIG_RT_CONSOLE_DEVICE_NAME="uart1"'),
        ("UART3", "UART1"),
    ])
    patch(TARGET_PATH / "rtconfig.h", [
        ("#define SOC_STM32H743ZI", "#define SOC_STM32H743II"),
        ("#define BOARD_STM32H743_NUCLEO", "#define BOARD_FK743M2_IIT6"),
        ("#define BOARD_SERIES_STM32_NUCLEO_144\n", ""),
        ('#define RT_CONSOLE_DEVICE_NAME "uart3"', '#define RT_CONSOLE_DEVICE_NAME "uart1"'),
        ("#define BSP_USING_UART3", "#define BSP_USING_UART1"),
    ])
    patch(TARGET_PATH / "board" / "Kconfig", [
        ("UART3", "UART1"),
    ])
    patch(TARGET_PATH / "board" / "board.c", [
        ("RCC_OscInitStruct.PLL.PLLM = 1;", "RCC_OscInitStruct.PLL.PLLM = 5;"),
        ("RCC_OscInitStruct.PLL.PLLN = 120;", "RCC_OscInitStruct.PLL.PLLN = 192;"),
        ("RCC_PLL1VCIRANGE_3", "RCC_PLL1VCIRANGE_2"),
        ("RCC_PERIPHCLK_USART3", "RCC_PERIPHCLK_USART1"),
        ("PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;",
         "PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;"),
    ])
    patch(TARGET_PATH / "board" / "CubeMX_Config" / "Inc" / "stm32h7xx_hal_conf.h", [
        ("#define HSE_VALUE    ((uint32_t)8000000)",
         "#define HSE_VALUE    ((uint32_t)25000000)"),
    ])

    # The source Nucleo BSP uses USART3 on PD8/PD9. Those pins are SDRAM
    # data lines on FK743M2! Replace its MSP implementation, not merely the
    # UART number, to avoid inadvertently taking over FMC pins.
    msp = TARGET_PATH / "board" / "CubeMX_Config" / "Src" / "stm32h7xx_hal_msp.c"
    msp.write_text((ROOT / "overlay" / "stm32h7xx_hal_msp.c").read_text(encoding="utf-8"),
                   encoding="utf-8")

    main = TARGET_PATH / "applications" / "main.c"
    main.write_text((ROOT / "overlay" / "main.c").read_text(encoding="utf-8"),
                    encoding="utf-8")
    print(f"Generated BSP: {TARGET_PATH}")
    print("Console: USART1 PA9(TX) PA10(RX), 115200 8N1 (3.3V TTL)")
    print("LED: PH7; HSE: 25 MHz; on-chip Flash: 2 MiB; AXI SRAM heap only")
    print("NOT YET SUPPORTED: SDRAM, QSPI, USB CDC/DFU, dynamic modules.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--no-clone", action="store_true",
                        help="Require third_party/rt-thread to already exist")
    args = parser.parse_args()
    if args.no_clone and not UPSTREAM.exists():
        parser.error(f"Missing checkout: {UPSTREAM}")
    checkout_upstream()
    create_bsp()
    return 0


if __name__ == "__main__":
    sys.exit(main())
