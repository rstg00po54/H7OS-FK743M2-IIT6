/*
 * H7OS first-stage board smoke test (NOT an application loader).
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define FK743M2_LED_PIN GET_PIN(H, 7)

int main(void)
{
    rt_uint32_t count = 0;
    rt_pin_mode(FK743M2_LED_PIN, PIN_MODE_OUTPUT);
    rt_kprintf("\nH7OS FK743M2-IIT6 stage-0 boot\n");
    rt_kprintf("STM32H743, USART1 console, PH7 LED\n");

    while (1)
    {
        rt_pin_write(FK743M2_LED_PIN, (count & 1U) ? PIN_LOW : PIN_HIGH);
        if ((count % 4U) == 0U)
            rt_kprintf("[H7OS] heartbeat %lu\n", (unsigned long)count);
        count++;
        rt_thread_mdelay(500);
    }
}
