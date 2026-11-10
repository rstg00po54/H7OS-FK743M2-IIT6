/*
 * FK743M2 USB OTG FS hardware configuration for RT-Thread v5.3.0.
 * SPDX-License-Identifier: Apache-2.0
 * USB: PA11 = DM, PA12 = DP, AF10. HSI48 supplies the USB FS clock.
 * No external UART / USB-TTL adapter is required for the MSH console.
 */
#include "main.h"

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef clk = {0};

    if (hpcd->Instance != USB_OTG_FS)
        return;

    clk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    clk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK)
    {
        /* Stop safely if the required USB 48 MHz clock cannot start.
         * This RT-Thread BSP has no CubeMX Error_Handler() symbol. */
        while (1)
        {
            __NOP();
        }
    }

    HAL_PWREx_EnableUSBVoltageDetector();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance != USB_OTG_FS)
        return;

    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
}


/* On-demand FK743M2-IIT6 W9825G6KH Bank1 GPIO assignment.
 * PA11/PA12 USB and PH7 status LED are intentionally untouched.
 */
void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *mem)
{
    GPIO_InitTypeDef gpio = {0};
    RCC_PeriphCLKInitTypeDef clk = {0};

    if (mem->Instance != FMC_SDRAM_DEVICE)
        return;

    clk.PeriphClockSelection = RCC_PERIPHCLK_FMC;
    clk.FmcClockSelection = RCC_FMCCLKSOURCE_D1HCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK)
        return;

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_FMC_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF12_FMC;

    /* PF0-5 A0-5, PF11 RAS, PF12-15 A6-9 */
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
               GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &gpio);

    /* PH2 CKE0, PH3 NE0, PH5 WE */
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOH, &gpio);

    /* PG0-2 A10-12, PG4-5 BA0-1, PG8 CLK, PG15 CAS */
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
               GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &gpio);

    /* PE0-1 DQM, PE7-15 D4-12 */
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
               GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio);

    /* PD0-1 D2-3, PD8-10 D13-15, PD14-15 D0-1 */
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
               GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio);
}
