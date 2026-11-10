"""Offline unit tests for the deterministic board-overlay transformation."""
import sys
import unittest
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from prepare import replace_exact, patch_shared_once, TAG, SOURCE, TARGET  # noqa: E402


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
        self.assertIn('finsh_set_prompt("cc@H7OS:~$ ");', main)
        self.assertIn("return finsh_prompt;", (ROOT / "scripts/prepare.py").read_text())


if __name__ == "__main__":
    unittest.main()
