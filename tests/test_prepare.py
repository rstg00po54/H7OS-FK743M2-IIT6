"""Offline unit tests for the deterministic board-overlay transformation."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from prepare import replace_exact, TAG, SOURCE, TARGET  # noqa: E402


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

    def test_overlay_pins(self):
        msp = (ROOT / "overlay/stm32h7xx_hal_msp.c").read_text()
        self.assertIn("GPIO_PIN_11 | GPIO_PIN_12", msp)
        self.assertIn("GPIO_AF10_OTG1_FS", msp)
        self.assertIn("HAL_PCD_MspInit", msp)
        self.assertNotIn("HAL_UART_MspInit", msp)
        self.assertNotIn("USART3", msp)
        main = (ROOT / "overlay/main.c").read_text()
        self.assertIn("GET_PIN(H, 7)", main)
        self.assertIn('rt_console_set_device("vcom")', main)
        self.assertIn('finsh_set_device("vcom")', main)


if __name__ == "__main__":
    unittest.main()
