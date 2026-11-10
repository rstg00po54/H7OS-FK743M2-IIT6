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
    # Refresh generated BSP templates without deleting the user's
    # packages/ downloads from 'pkgs --update'. A full rmtree here
    # would force a new HAL/CMSIS download every time we fix an overlay.
    shutil.copytree(source, TARGET_PATH, dirs_exist_ok=True)

    patch(TARGET_PATH / "Kconfig", [
        ("SOC_STM32H743ZI", "SOC_STM32H743II"),
        ("BOARD_STM32H743_NUCLEO", "BOARD_FK743M2_IIT6"),
        ("    select BOARD_SERIES_STM32_NUCLEO_144\n", ""),
    ])
    patch(TARGET_PATH / ".config", [
        ("CONFIG_SOC_STM32H743ZI=y", "CONFIG_SOC_STM32H743II=y"),
        ("CONFIG_BOARD_STM32H743_NUCLEO=y", "CONFIG_BOARD_FK743M2_IIT6=y"),
        ("CONFIG_BOARD_SERIES_STM32_NUCLEO_144=y\n", ""),
        ('CONFIG_RT_CONSOLE_DEVICE_NAME="uart3"', 'CONFIG_RT_CONSOLE_DEVICE_NAME="vcom"'),
        ("CONFIG_BSP_USING_UART=y", "# CONFIG_BSP_USING_UART is not set"),
        ("CONFIG_BSP_USING_UART3=y", "# CONFIG_BSP_USING_UART3 is not set"),
        ("# CONFIG_RT_USING_USB_DEVICE is not set",
         """CONFIG_RT_USING_USB=y
CONFIG_RT_USING_USB_DEVICE=y
CONFIG_RT_USBD_THREAD_STACK_SZ=4096
CONFIG_USB_VENDOR_ID=0x0FFE
CONFIG_USB_PRODUCT_ID=0x0001
# CONFIG_RT_USB_DEVICE_COMPOSITE is not set
CONFIG__RT_USB_DEVICE_CDC=y
CONFIG_RT_USB_DEVICE_CDC=y
CONFIG_RT_VCOM_TASK_STK_SIZE=1024
CONFIG_RT_CDC_RX_BUFSIZE=512
CONFIG_RT_VCOM_MANUFACTURER_STRING="H7OS Dev"
CONFIG_RT_VCOM_PRODUCT_STRING="H7OS USB Shell"
CONFIG_RT_VCOM_SERNO="H7OSFK74300001"
CONFIG_RT_VCOM_SER_LEN=14
CONFIG_RT_VCOM_TX_TIMEOUT=1000"""),
        ("# end of Hardware Drivers Config",
         """CONFIG_BSP_USING_USBD=y
CONFIG_BSP_USBD_TYPE_FS=y
CONFIG_BSP_USBD_PHY_EMBEDDED=y
# end of Hardware Drivers Config"""),
    ])
    patch(TARGET_PATH / "rtconfig.h", [
        ("#define SOC_STM32H743ZI", "#define SOC_STM32H743II"),
        ("#define BOARD_STM32H743_NUCLEO", "#define BOARD_FK743M2_IIT6"),
        ("#define BOARD_SERIES_STM32_NUCLEO_144\n", ""),
        ('#define RT_CONSOLE_DEVICE_NAME "uart3"', '#define RT_CONSOLE_DEVICE_NAME "vcom"'),
        ("#define BSP_USING_UART3\n", ""),
        ("#define BSP_USING_UART\n", ""),
        ("#define BSP_STM32_UART_V1_TX_TIMEOUT 6000\n", ""),
        ("/* Using USB legacy version */",
         """/* Using USB legacy version */
#define RT_USING_USB
#define RT_USING_USB_DEVICE
#define RT_USBD_THREAD_STACK_SZ 4096
#define USB_VENDOR_ID 0x0FFE
#define USB_PRODUCT_ID 0x0001
#define _RT_USB_DEVICE_CDC
#define RT_USB_DEVICE_CDC
#define RT_VCOM_TASK_STK_SIZE 1024
#define RT_CDC_RX_BUFSIZE 512
#define RT_VCOM_MANUFACTURER_STRING "H7OS Dev"
#define RT_VCOM_PRODUCT_STRING "H7OS USB Shell"
#define RT_VCOM_SERNO "H7OSFK74300001"
#define RT_VCOM_SER_LEN 14
#define RT_VCOM_TX_TIMEOUT 1000"""),
        ("/* end of Hardware Drivers Config */",
         """#define BSP_USING_USBD
#define BSP_USBD_TYPE_FS
#define BSP_USBD_PHY_EMBEDDED
/* end of Hardware Drivers Config */"""),
    ])
    patch(TARGET_PATH / "board" / "Kconfig", [
        ('menu "Board extended module Drivers"',
         'config BSP_USING_USBD\n'
         '    bool "Enable USB FS CDC Shell"\n'
         '    default y\n'
         '    select RT_USING_USB_DEVICE\n'
         '    select BSP_USBD_TYPE_FS\n'
         '    select BSP_USBD_PHY_EMBEDDED\n\n'
         'menu "Board extended module Drivers"'),
    ])
    patch(TARGET_PATH / "board" / "board.c", [
        ("RCC_OscInitStruct.PLL.PLLM = 1;", "RCC_OscInitStruct.PLL.PLLM = 5;"),
        ("RCC_OscInitStruct.PLL.PLLN = 120;", "RCC_OscInitStruct.PLL.PLLN = 192;"),
        ("RCC_PLL1VCIRANGE_3", "RCC_PLL1VCIRANGE_2"),
        ("RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;",
         "RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;"),
        ("RCC_OscInitStruct.HSEState = RCC_HSE_ON;",
         "RCC_OscInitStruct.HSEState = RCC_HSE_ON;\n    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;"),
        ("RCC_PERIPHCLK_USART3", "RCC_PERIPHCLK_USB"),
        ("PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;",
         "PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;"),
    ])
    patch(TARGET_PATH / "board" / "CubeMX_Config" / "Inc" / "stm32h7xx_hal_conf.h", [
        ("#define HSE_VALUE    ((uint32_t)8000000)",
         "#define HSE_VALUE    ((uint32_t)25000000)"),
        ("/* #define HAL_PCD_MODULE_ENABLED   */",
         "#define HAL_PCD_MODULE_ENABLED"),
    ])

    # Do not initialize Nucleo's USART3 on PD8/PD9: these pins are
    # occupied by the FK743M2 SDRAM. USB OTG FS uses PA11/PA12 instead.
    msp = TARGET_PATH / "board" / "CubeMX_Config" / "Src" / "stm32h7xx_hal_msp.c"
    msp.write_text((ROOT / "overlay" / "stm32h7xx_hal_msp.c").read_text(encoding="utf-8"),
                   encoding="utf-8")


    # USB CDC registers a virtual rt_serial_device, so RT_USING_SERIAL
    # must stay enabled. But that does not imply a physical STM32 USART
    # exists. Disable the generic USART driver and its early init when
    # this target has BSP_USING_UART disabled.
    drivers_sconscript = BSP_ROOT / "libraries" / "HAL_Drivers" / "drivers" / "SConscript"
    patch(drivers_sconscript, [
        ("if GetDepend(['RT_USING_SERIAL']):",
         "if GetDepend(['BSP_USING_UART']):"),
    ])
    drv_common = BSP_ROOT / "libraries" / "HAL_Drivers" / "drv_common.c"
    patch(drv_common, [
        ("#ifdef RT_USING_SERIAL", "#ifdef BSP_USING_UART"),
    ])

    main = TARGET_PATH / "applications" / "main.c"
    main.write_text((ROOT / "overlay" / "main.c").read_text(encoding="utf-8"),
                    encoding="utf-8")
    print(f"Generated BSP: {TARGET_PATH}")
    print("Console: USB OTG FS CDC (PA11/PA12), Linux /dev/ttyACM*")
    print("LED: PH7; HSE: 25 MHz; on-chip Flash: 2 MiB; AXI SRAM heap only")
    print("USB CDC configured but not yet tested on board; ROM DFU is separate.")
    print("NOT YET SUPPORTED: SDRAM, QSPI, runtime DFU, dynamic modules.")


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
