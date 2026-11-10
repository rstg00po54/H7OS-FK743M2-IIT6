/*
 * H7OS USB-only stage-0 board smoke test (NOT an application loader).
 * SPDX-License-Identifier: Apache-2.0
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#include <shell.h>
#endif

#define FK743M2_LED_PIN GET_PIN(H, 7)
#define H7OS_PWM_STEPS 100U
#define H7OS_PWM_TICK_HZ 10000U /* 100 Hz LED PWM, 1% steps */

/* PH7 has no timer PWM alternate function on STM32H743.
 * TIM6 supplies update interrupts; the ISR drives PH7's atomic BSRR.
 * This uses TIM6 exclusively until a different LED driver is selected.
 */
enum h7os_led_mode
{
    H7OS_LED_BLINK,
    H7OS_LED_ON,
    H7OS_LED_OFF,
    H7OS_LED_PWM,
};

static volatile enum h7os_led_mode g_led_mode = H7OS_LED_BLINK;
static volatile rt_uint32_t g_h7os_led_toggles;
static volatile rt_uint32_t g_led_brightness = 50U;
static volatile rt_uint32_t g_pwm_phase;
static rt_bool_t g_tim6_ready;

/* The board's LED is assumed active-high. If the actual board has an
 * active-low LED, swap these two BSRR writes. */
static void h7os_led_set(rt_bool_t on)
{
    rt_pin_write(FK743M2_LED_PIN, on ? PIN_HIGH : PIN_LOW);
}

void TIM6_DAC_IRQHandler(void)
{
    if ((TIM6->SR & TIM_SR_UIF) != 0U)
    {
        TIM6->SR = 0U;
        if (g_led_mode != H7OS_LED_PWM)
        {
            return;
        }

        /* 100 levels, 100 Hz cycle; no RT-Thread API calls from ISR. */
        if (g_pwm_phase >= (H7OS_PWM_STEPS - 1U))
            g_pwm_phase = 0U;
        else
            g_pwm_phase++;

        if (g_pwm_phase < g_led_brightness)
            GPIOH->BSRR = GPIO_PIN_7;
        else
            GPIOH->BSRR = (rt_uint32_t)GPIO_PIN_7 << 16U;
    }
}

static void h7os_led_pwm_init(void)
{
    RCC_ClkInitTypeDef clocks = {0};
    rt_uint32_t flash_latency;
    rt_uint32_t timclk;

    __HAL_RCC_TIM6_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clocks, &flash_latency);
    timclk = HAL_RCC_GetPCLK1Freq();

    /* With this H743 BSP, TIMPRE is left at reset default and
     * APB1 prescaler != 1 makes the timer clock 2 x PCLK1. */
    if (clocks.APB1CLKDivider != RCC_APB1_DIV1)
        timclk *= 2U;

    if (timclk < H7OS_PWM_TICK_HZ)
    {
        g_tim6_ready = RT_FALSE;
        return;
    }

    TIM6->CR1 = 0U;
    TIM6->DIER = 0U;
    TIM6->PSC = (timclk / 1000000U) - 1U;
    TIM6->ARR = (1000000U / H7OS_PWM_TICK_HZ) - 1U;
    TIM6->CNT = 0U;
    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR = 0U;
    NVIC_SetPriority(TIM6_DAC_IRQn, 8U);
    NVIC_ClearPendingIRQ(TIM6_DAC_IRQn);
    g_tim6_ready = RT_TRUE;
}

static void h7os_led_change_mode(enum h7os_led_mode mode, rt_uint32_t duty)
{
    rt_base_t level = rt_hw_interrupt_disable();

    TIM6->DIER &= ~TIM_DIER_UIE;
    TIM6->CR1 &= ~TIM_CR1_CEN;
    NVIC_DisableIRQ(TIM6_DAC_IRQn);
    NVIC_ClearPendingIRQ(TIM6_DAC_IRQn);

    g_led_mode = mode;
    g_led_brightness = duty;
    g_pwm_phase = 0U;

    if (mode == H7OS_LED_PWM && duty > 0U && duty < 100U && g_tim6_ready)
    {
        TIM6->CNT = 0U;
        TIM6->SR = 0U;
        TIM6->DIER |= TIM_DIER_UIE;
        NVIC_EnableIRQ(TIM6_DAC_IRQn);
        TIM6->CR1 |= TIM_CR1_CEN;
    }
    rt_hw_interrupt_enable(level);

    /* No timer interrupts are needed for 0% or 100% duty cycle. */
    if (mode == H7OS_LED_ON || (mode == H7OS_LED_PWM && duty == 100U))
        h7os_led_set(RT_TRUE);
    else if (mode != H7OS_LED_BLINK)
        h7os_led_set(RT_FALSE);
}

#ifdef RT_USING_FINSH
static const char *h7os_led_mode_name(enum h7os_led_mode mode)
{
    switch (mode)
    {
    case H7OS_LED_ON:   return "on";
    case H7OS_LED_OFF:  return "off";
    case H7OS_LED_PWM:  return "pwm";
    default:            return "blink";
    }
}

static int led(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && rt_strcmp(argv[1], "status") == 0))
    {
        rt_kprintf("PH7 LED: mode=%s, duty=%lu%%, timer=%s\n",
                   h7os_led_mode_name(g_led_mode),
                   (unsigned long)g_led_brightness,
                   g_tim6_ready ? "TIM6" : "unavailable");
        return 0;
    }

    if (argc == 2 && rt_strcmp(argv[1], "on") == 0)
    {
        h7os_led_change_mode(H7OS_LED_ON, 100U);
        return 0;
    }
    if (argc == 2 && rt_strcmp(argv[1], "off") == 0)
    {
        h7os_led_change_mode(H7OS_LED_OFF, 0U);
        return 0;
    }
    if (argc == 2 && rt_strcmp(argv[1], "blink") == 0)
    {
        h7os_led_change_mode(H7OS_LED_BLINK, 50U);
        return 0;
    }
    if (argc == 3 && rt_strcmp(argv[1], "pwm") == 0)
    {
        const char *p = argv[2];
        rt_uint32_t duty = 0U;
        if (*p == '\0')
            goto usage;
        while (*p != '\0')
        {
            if (*p < '0' || *p > '9')
                goto usage;
            duty = duty * 10U + (rt_uint32_t)(*p - '0');
            if (duty > 100U)
                goto usage;
            p++;
        }
        if (!g_tim6_ready && duty != 0U && duty != 100U)
        {
            rt_kprintf("TIM6 PWM is unavailable\n");
            return -RT_ERROR;
        }
        h7os_led_change_mode(H7OS_LED_PWM, duty);
        return 0;
    }

usage:
    rt_kprintf("Usage: led on | off | blink | pwm 0..100 | status\n");
    return -RT_EINVAL;
}
MSH_CMD_EXPORT(led, Control PH7 LED with TIM6 software PWM);

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
    h7os_led_pwm_init();

    /* Device init creates 'vcom' after USB class registration. Bind
     * after init, not in rt_hw_board_init where USB is not ready yet. */
    while (!console_ready)
    {
        if (rt_device_find("vcom") != RT_NULL)
        {
            rt_console_set_device("vcom");
#ifdef RT_USING_FINSH
            finsh_set_prompt("cc@H7OS:~$ ");
            finsh_set_device("vcom");
#endif
            console_ready = RT_TRUE;
            rt_kprintf("\nH7OS FK743M2-IIT6 USB stage-0 boot\n");
            rt_kprintf("STM32H743 / USB CDC vcom / PH7 LED\n");
            break;
        }

        h7os_led_set(RT_TRUE);
        rt_thread_mdelay(100);
        h7os_led_set(RT_FALSE);
        rt_thread_mdelay(100);
    }

    while (1)
    {
        if (g_led_mode == H7OS_LED_BLINK)
        {
            led_state = !led_state;
            h7os_led_set(led_state);
            g_h7os_led_toggles++;
        }
        rt_thread_mdelay(500);
    }
}
