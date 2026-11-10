#!/usr/bin/env python3
"""Integrate early one-shot ROM DFU check into the downloaded H743 Reset_Handler.

Run after 'pkgs --update' and before SCons. The check must execute BEFORE
ExitRun0Mode / SystemInit and C runtime .data/.bss initialization so the
one-shot DTCM marker cannot be disturbed by regular board bring-up.
"""
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
STARTUP = (
    ROOT / "third_party/rt-thread/bsp/stm32/libraries"
    / "stm32h7_cmsis_driver-latest/Source/Templates/gcc/startup_stm32h743xx.s"
)

HOOK = "  bl  h7os_early_dfu_check  /* H7OS: early ROM DFU flag check */"
ANCHOR = "  ldr   sp, =_estack      /* set stack pointer */"


def patch_reset_handler(text: str) -> str:
    """Insert a single call at the H743 reset vector; fail on unknown files."""
    if text.count(HOOK) == 1:
        return text
    if HOOK in text or text.count(ANCHOR) != 1:
        raise ValueError("Unexpected CMSIS startup Reset_Handler source")
    if text.count("Reset_Handler:") != 1 or "bl  ExitRun0Mode" not in text:
        raise ValueError("H743 Reset_Handler does not match pinned CMSIS template")
    start = text.index("Reset_Handler:")
    anchor = text.index(ANCHOR, start)
    exitrun = text.index("bl  ExitRun0Mode", start)
    if not start < anchor < exitrun:
        raise ValueError("Incorrect reset setup order")
    return text.replace(ANCHOR, ANCHOR + "\n" + HOOK, 1)


def main() -> int:
    if not STARTUP.is_file():
        print(f"Missing downloaded H743 startup assembly: {STARTUP}", file=sys.stderr)
        print("Run bash scripts/setup_deps.sh before building.", file=sys.stderr)
        return 1
    before = STARTUP.read_text(encoding="utf-8")
    after = patch_reset_handler(before)
    if after != before:
        STARTUP.write_text(after, encoding="utf-8")
        print("Patched CMSIS Reset_Handler for early DFU handoff")
    else:
        print("CMSIS Reset_Handler DFU hook already installed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
