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
