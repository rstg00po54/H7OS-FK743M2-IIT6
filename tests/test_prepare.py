"""Offline unit tests for the deterministic board-overlay transformation."""
import sys
import unittest
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from prepare import replace_exact, patch_shared_once, TAG, SOURCE, TARGET
from patch_startup import patch_reset_handler  # noqa: E402


class PrepareTests(unittest.TestCase):
    def test_pin_upstream_version(self):
        self.assertEqual(TAG, "v5.3.0")
        self.assertEqual(SOURCE, "stm32h743-st-nucleo")
        self.assertEqual(TARGET, "stm32h743-fk743m2-iit6")

    def test_exact_replacement_fails_closed(self):
        with self.assertRaises(ValueError):
            replace_exact("STM32H743", "unknown text", "replacement", "test")

    def test_exact_replacement_success(self):
        self.assertEqual(
            replace_exact("USART3 on PD8", "USART3", "USART1", "board"),
            "USART1 on PD8"
        )

    def test_regenerating_does_not_remove_pkgs_cache(self):
        prepare = (ROOT / "scripts/prepare.py").read_text()
        self.assertIn("dirs_exist_ok=True", prepare)
        self.assertNotIn("shutil.rmtree(TARGET_PATH)", prepare)

    def test_usb_serial_does_not_require_hardware_usart(self):
        prepare = (ROOT / "scripts/prepare.py").read_text()
        self.assertIn("patch_shared_once(drivers_sconscript", prepare)
        self.assertIn("patch_shared_once(drv_common", prepare)

    def test_shared_patch_is_idempotent(self):
        with tempfile.TemporaryDirectory() as d:
            driver = Path(d) / "drv_common.c"
            driver.write_text("#ifdef RT_USING_SERIAL\\n#endif\\n")
            patch_shared_once(driver, "#ifdef RT_USING_SERIAL", "#ifdef BSP_USING_UART")
            patch_shared_once(driver, "#ifdef RT_USING_SERIAL", "#ifdef BSP_USING_UART")
            self.assertEqual(driver.read_text(), "#ifdef BSP_USING_UART\\n#endif\\n")

    def test_early_reset_hook_before_system_init(self):
        startup = ("Reset_Handler:\n"
                   "  ldr   sp, =_estack      /* set stack pointer */\n"
                   "  bl  ExitRun0Mode\n"
                   "  bl  SystemInit\n")
        patched = patch_reset_handler(startup)
        self.assertEqual(patched.count("bl  h7os_early_dfu_check"), 1)
        self.assertLess(patched.index("h7os_early_dfu_check"),
                        patched.index("ExitRun0Mode"))
        self.assertEqual(patch_reset_handler(patched), patched)

    def test_unknown_reset_handler_fails(self):
        with self.assertRaises(ValueError):
            patch_reset_handler("Reset_Handler:\n  bl  SystemInit\n")

    def test_tmpfs_config_and_null_safe_mount(self):
        prepare = (ROOT / "scripts/prepare.py").read_text()
        for symbol in ("CONFIG_RT_USING_DFS=y", "CONFIG_DFS_USING_POSIX=y",
                       "CONFIG_DFS_USING_WORKDIR=y", "CONFIG_RT_USING_DFS_V1=y",
                       "CONFIG_RT_USING_DFS_TMPFS=y", "# CONFIG_RT_USING_DFS_DEVFS is not set",
                       "#define RT_USING_DFS_TMPFS"):
            self.assertIn(symbol, prepare)
        self.assertIn('iter->dev_id ? iter->dev_id->parent.name', prepare)

    def test_tmpfs_is_mounted_before_shell_binding(self):
        main = (ROOT / "overlay/main.c").read_text()
        self.assertIn('dfs_mount(RT_NULL, "/", "tmp", 0, RT_NULL)', main)
        self.assertIn('chdir("/home/cc")', main)
        self.assertLess(main.index('g_fs_ready = h7os_mount_root();'),
                        main.index('finsh_set_device("vcom");'))
        self.assertIn('rt_kprintf("Filesystem: %s', main)

    def test_overlay_pins(self):
        msp = (ROOT / "overlay/stm32h7xx_hal_msp.c").read_text()
        self.assertIn("GPIO_PIN_11 | GPIO_PIN_12", msp)
        self.assertIn("GPIO_AF10_OTG1_FS", msp)
        self.assertIn("HAL_PCD_MspInit", msp)
        self.assertNotIn("        Error_Handler();", msp)
        self.assertNotIn("HAL_UART_MspInit", msp)
        self.assertNotIn("USART3", msp)
        main = (ROOT / "overlay/main.c").read_text()
        self.assertIn("GET_PIN(H, 7)", main)
        self.assertIn('rt_console_set_device("vcom")', main)
        self.assertIn('finsh_set_device("vcom")', main)
        self.assertNotIn('rt_kprintf("[H7OS] heartbeat', main)
        self.assertIn("MSH_CMD_EXPORT(h7os", main)
        self.assertIn("g_h7os_led_toggles++", main)
        self.assertIn("TIM6_DAC_IRQHandler", main)
        self.assertIn("MSH_CMD_EXPORT(led", main)
        self.assertIn("H7OS_PWM_TICK_HZ 10000U", main)
        self.assertIn("on ? PIN_LOW : PIN_HIGH", main)
        self.assertIn("GPIOH->BSRR = (rt_uint32_t)GPIO_PIN_7 << 16U;", main)
        self.assertIn("H7OS_DFU_MAGIC_ADDR 0x2001FFFCUL", main)
        self.assertIn("H7OS_ROM_BOOT_ENTRY 0x1FF09800UL", main)
        self.assertIn("MSH_CMD_EXPORT(dfu", main)
        self.assertIn("__attribute__((naked)) void h7os_early_dfu_check", main)
        self.assertIn("NVIC_SystemReset();", main)
        self.assertIn("USB_OTG_DCTL_SDIS", main)
        self.assertIn("rt_thread_mdelay(150)", main)
        self.assertIn("patch_shared_once(", (ROOT / "scripts/prepare.py").read_text())
        self.assertIn('finsh_set_prompt("cc@H7OS:~$ ");', main)
        self.assertIn("return finsh_prompt;", (ROOT / "scripts/prepare.py").read_text())


if __name__ == "__main__":
    unittest.main()
