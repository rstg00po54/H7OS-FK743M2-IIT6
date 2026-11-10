# H7OS USB-only boot and console

**Status: source integration only; physical board testing is pending.** The FK743M2 target does not require a separate UART adapter.

## 1. Two different USB modes

| Mode | USB appearance | Purpose |
| --- | --- | --- |
| STM32 ROM bootloader | 0483:df11 (DFU) | Rescue and flash the internal Flash |
| H7OS application | USB CDC ACM (`/dev/ttyACM0` on Ubuntu) | MSH Shell, logging; eventually file uploads |

**Do not expect DFU and CDC simultaneously from the ROM Bootloader.**
Once programmed, exit BOOT0/ROM DFU and reset the device to run H7OS.
This project does **not** yet offer USB firmware-upgrade commands or automatic runtime DFU switch.

## 2. USB hardware

- STM32H743 USB OTG FS peripheral: PA11 USB_DM, PA12 USB_DP (AF10)
- OTG FS HSI48 clock enabled in `SystemClock_Config`
- USB peripheral PCD configured using RT-Thread v5.3.0 legacy USB Device CDC stack
- STM32 USB PCD MSP supplies OTG_FS clock and USB voltage detector.
- CDC device is registered as `vcom`; `main()` binds RT-Thread console and FinSH to that device after device initialization.
- The generic USART3 configuration was removed: PD8 and PD9 are used by SDRAM on this board.
- **Important:** A cable providing power but no data cannot enumerate the USB device.

## 3. Host troubleshooting

```bash
lsusb
dmesg -w
ls -l /dev/ttyACM*
sudo apt install picocom
picocom -b 115200 /dev/ttyACM0
```

The 115200 value in a CDC terminal is only host line coding; no physical UART baud clock exists.

If `lsusb` shows `0483:df11`, it is in ROM DFU, not the USB CDC application.

If no CDC device appears: verify USB cable, board boot pins, PCD startup/USB 48 MHz clock and USB D+/D- wiring. LED PH7 heartbeat may distinguish firmware execution from boot failures.

## 4. USB vendor/product IDs

Development-only test configuration uses RT-Thread's example VID/PID `0x0FFE:0x0001`; this is **not** an assigned production USB identity and must be replaced with a properly authorized ID before distributing a USB product.

## 5. Restrictions

- Hardware boot and USB enumeration are not verified yet.
- Dynamic module loader, QSPI storage and application upload protocol are not implemented yet.
- Do not rely on unplugging/reset as a complete recovery method if BOOT0 or SWD is inaccessible.
