/*
 * H7OS USB-only stage-0 board smoke test (NOT an application loader).
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif

#define FK743M2_LED_PIN GET_PIN(H, 7)

/* Updated by the main thread; 32-bit aligned loads are atomic on Cortex-M7. */
static volatile rt_uint32_t g_h7os_led_toggles;

#ifdef RT_USING_FINSH
/* Query liveness explicitly instead of continually interleaving logs with MSH. */
static int h7os(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    rt_kprintf("H7OS FK743M2-IIT6: alive, ticks=%lu, LED toggles=%lu\n",
               (unsigned long)rt_tick_get(),
               (unsigned long)g_h7os_led_toggles);
    return 0;
}
MSH_CMD_EXPORT(h7os, Show H7OS system heartbeat);
#endif

int main(void)
{
    rt_bool_t led_state = RT_FALSE;
    rt_bool_t console_ready = RT_FALSE;

    rt_pin_mode(FK743M2_LED_PIN, PIN_MODE_OUTPUT);

    /* Device init creates 'vcom' after USB class registration. Bind
     * after init, not in rt_hw_board_init where USB is not ready yet. */
    while (!console_ready)
    {
        if (rt_device_find("vcom") != RT_NULL)
        {
            rt_console_set_device("vcom");
#ifdef RT_USING_FINSH
            finsh_set_device("vcom");
#endif
            console_ready = RT_TRUE;
            rt_kprintf("\nH7OS FK743M2-IIT6 USB stage-0 boot\n");
            rt_kprintf("STM32H743 / USB CDC vcom / PH7 LED\n");
            break;
        }

        rt_pin_write(FK743M2_LED_PIN, PIN_HIGH);
        rt_thread_mdelay(100);
        rt_pin_write(FK743M2_LED_PIN, PIN_LOW);
        rt_thread_mdelay(100);
    }

    while (1)
    {
        led_state = !led_state;
        rt_pin_write(FK743M2_LED_PIN, led_state ? PIN_HIGH : PIN_LOW);
        g_h7os_led_toggles++;
        rt_thread_mdelay(500);
    }
}
