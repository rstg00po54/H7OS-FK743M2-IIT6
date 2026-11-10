/*
 * H7OS W9825G6KH standalone FMC SDRAM driver.
 * Never initializes at boot. Run "sdram init" in USB MSH.
 * Memory remains outside the heap/tmpfs until full hardware validation.
 * Every test command writes DESTRUCTIVELY to SDRAM.
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <finsh.h>
#include "h7os_sdram.h"

#define H7OS_SDRAM_WORDS (H7OS_SDRAM_SIZE / 4U)
#define SDRAM_CMD_TIMEOUT 1000U

static SDRAM_HandleTypeDef hsdram;
static rt_bool_t sdram_ready;
static rt_bool_t sdram_attempted;
static const char *sdram_error = "not initialized";

rt_bool_t h7os_sdram_is_ready(void) { return sdram_ready; }

static void sdram_mpu_configure(void)
{
    MPU_Region_InitTypeDef region = {0};
    rt_uint32_t ctrl = MPU->CTRL;

    /* Region 7 reserved for SDRAM, Normal non-cacheable, XN.
     * The original MPU configuration for other regions is retained.
     */
    HAL_MPU_Disable();
    region.Enable = MPU_REGION_ENABLE;
    region.Number = MPU_REGION_NUMBER7;
    region.BaseAddress = H7OS_SDRAM_BASE;
    region.Size = MPU_REGION_SIZE_32MB;
    region.SubRegionDisable = 0U;
    region.TypeExtField = MPU_TEX_LEVEL1;
    region.AccessPermission = MPU_REGION_FULL_ACCESS;
    region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    region.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    region.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&region);
    HAL_MPU_Enable((ctrl & MPU_CTRL_ENABLE_Msk) ?
                   (ctrl & ~MPU_CTRL_ENABLE_Msk) : MPU_PRIVILEGED_DEFAULT);
    __DSB();
    __ISB();
}

static HAL_StatusTypeDef sdram_cmd(rt_uint32_t mode, rt_uint32_t count,
                                   rt_uint32_t value)
{
    FMC_SDRAM_CommandTypeDef cmd = {0};
    cmd.CommandMode = mode;
    cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    cmd.AutoRefreshNumber = count;
    cmd.ModeRegisterDefinition = value;
    return HAL_SDRAM_SendCommand(&hsdram, &cmd, SDRAM_CMD_TIMEOUT);
}

static rt_bool_t sdram_hw_init(void)
{
    FMC_SDRAM_TimingTypeDef t = {0};
    /* MRD: burst=1, sequential, CL3, normal, single write. */
    const rt_uint32_t mrd = (3U << 4) | (1U << 9);

    if (HAL_RCC_GetHCLKFreq() != 240000000U)
    {
        sdram_error = "unsupported HCLK (expected 240MHz)";
        return RT_FALSE;
    }
    sdram_mpu_configure();
    hsdram.Instance = FMC_SDRAM_DEVICE;
    hsdram.Init.SDBank = FMC_SDRAM_BANK1;
    hsdram.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;
    hsdram.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
    hsdram.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
    hsdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    hsdram.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
    hsdram.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    hsdram.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
    hsdram.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
    hsdram.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
    t.LoadToActiveDelay = 2;
    t.ExitSelfRefreshDelay = 9;
    t.SelfRefreshTime = 6;
    t.RowCycleDelay = 8;
    t.WriteRecoveryTime = 3;
    t.RPDelay = 3;
    t.RCDDelay = 3;

    if (HAL_SDRAM_Init(&hsdram, &t) != HAL_OK)
    {
        sdram_error = "HAL_SDRAM_Init";
        return RT_FALSE;
    }
    /* JEDEC: enable clock, wait >100us, PALL, 8 refresh, mode register.
     * Fixed refresh COUNT 761 from reference at SDCLK=120MHz
     * (~6.34us vs required 7.81us maximum).
     */
    if (sdram_cmd(FMC_SDRAM_CMD_CLK_ENABLE, 1U, 0U) != HAL_OK)
    {
        sdram_error = "SDCLK";
        return RT_FALSE;
    }
    rt_thread_mdelay(1);
    if (sdram_cmd(FMC_SDRAM_CMD_PALL, 1U, 0U) != HAL_OK ||
        sdram_cmd(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8U, 0U) != HAL_OK ||
        sdram_cmd(FMC_SDRAM_CMD_LOAD_MODE, 1U, mrd) != HAL_OK)
    {
        sdram_error = "JEDEC sequence";
        return RT_FALSE;
    }
    if (HAL_SDRAM_ProgramRefreshRate(&hsdram, 761U) != HAL_OK)
    {
        sdram_error = "refresh setup";
        return RT_FALSE;
    }
    __DSB();
    return RT_TRUE;
}

static rt_bool_t sdram_data_test(void)
{
    volatile rt_uint32_t *p = (volatile rt_uint32_t *)H7OS_SDRAM_BASE;
    rt_uint32_t bit, value;
    for (bit = 0; bit < 32U; ++bit)
    {
        value = 1UL << bit;
        p[0] = value;
        __DSB();
        if (p[0] != value)
        {
            rt_kprintf("SDRAM data bit %lu FAIL: got 0x%08lx\n",
                       (unsigned long)bit, (unsigned long)p[0]);
            return RT_FALSE;
        }
    }
    return RT_TRUE;
}

static rt_bool_t sdram_address_test(void)
{
    volatile rt_uint32_t *p = (volatile rt_uint32_t *)H7OS_SDRAM_BASE;
    rt_uint32_t bit, other, offset = 0U;
    const rt_uint32_t pattern = 0xAAAAAAAAUL, anti = 0x55555555UL;
    p[0] = pattern;
    for (bit = 0U; bit < 23U; ++bit)
        p[1UL << bit] = anti;
    __DSB();
    if (p[0] != pattern)
    {
        rt_kprintf("SDRAM base address alias FAIL\n");
        return RT_FALSE;
    }
    for (bit = 0U; bit < 23U; ++bit)
    {
        offset = 1UL << bit;
        if (p[offset] != anti)
            goto failed;
        p[offset] = pattern;
        __DSB();
        if (p[0] != pattern)
            goto failed;
        for (other = 0U; other < 23U; ++other)
            if (other != bit && p[1UL << other] != anti)
                goto failed;
        p[offset] = anti;
    }
    p[H7OS_SDRAM_WORDS - 1U] = 0xDEADBEEFUL;
    __DSB();
    if (p[H7OS_SDRAM_WORDS - 1U] != 0xDEADBEEFUL)
    {
        rt_kprintf("SDRAM last address FAIL\n");
        return RT_FALSE;
    }
    return RT_TRUE;
failed:
    rt_kprintf("SDRAM address bit %lu FAIL at 0x%08lx\n",
               (unsigned long)bit,
               (unsigned long)(H7OS_SDRAM_BASE + offset * 4U));
    return RT_FALSE;
}

static rt_bool_t sdram_full_test(void)
{
    volatile rt_uint32_t *p = (volatile rt_uint32_t *)H7OS_SDRAM_BASE;
    rt_uint32_t pass, i, value, actual;
    for (pass = 0; pass < 2U; ++pass)
    {
        for (i = 0U; i < H7OS_SDRAM_WORDS; ++i)
        {
            value = i ^ 0xA5A55A5AUL;
            p[i] = pass ? ~value : value;
            if ((i & 0xFFFFUL) == 0xFFFFUL)
                rt_thread_mdelay(1);
        }
        __DSB();
        for (i = 0U; i < H7OS_SDRAM_WORDS; ++i)
        {
            value = i ^ 0xA5A55A5AUL;
            value = pass ? ~value : value;
            actual = p[i];
            if (actual != value)
            {
                rt_kprintf("SDRAM FAIL @0x%08lx expected %08lx got %08lx\n",
                           (unsigned long)(H7OS_SDRAM_BASE + i * 4U),
                           (unsigned long)value, (unsigned long)actual);
                return RT_FALSE;
            }
            if ((i & 0xFFFFUL) == 0xFFFFUL)
                rt_thread_mdelay(1);
        }
        rt_kprintf("SDRAM full pass %lu/2 OK\n",
                   (unsigned long)(pass + 1U));
    }
    return RT_TRUE;
}

#ifdef RT_USING_FINSH
static int sdram(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && rt_strcmp(argv[1], "status") == 0))
    {
        rt_kprintf("SDRAM W9825G6KH 32MiB 0xC0000000: %s\n",
                   sdram_ready ? "ready (not heap)" :
                   sdram_attempted ? sdram_error : "not initialized");
        return 0;
    }
    if (argc == 2 && rt_strcmp(argv[1], "init") == 0)
    {
        if (sdram_ready)
        {
            rt_kprintf("SDRAM already initialized\n");
            return 0;
        }
        if (sdram_attempted)
        {
            rt_kprintf("Previous init failed (%s); reboot before retry\n",
                       sdram_error);
            return -RT_ERROR;
        }
        sdram_attempted = RT_TRUE;
        rt_kprintf("Initializing FMC Bank1 (SDCLK=120MHz)...\n");
        if (!sdram_hw_init())
        {
            rt_kprintf("SDRAM FAILED: %s\n", sdram_error);
            return -RT_ERROR;
        }
        if (!sdram_data_test() || !sdram_address_test())
        {
            sdram_error = "memory readback failed";
            rt_kprintf("SDRAM FAILED: %s\n", sdram_error);
            return -RT_ERROR;
        }
        sdram_ready = RT_TRUE;
        rt_kprintf("SDRAM READY: 32 MiB (outside AXI heap)\n");
        return 0;
    }
    if (argc == 3 && rt_strcmp(argv[1], "test") == 0)
    {
        if (!sdram_ready)
        {
            rt_kprintf("Run 'sdram init' first\n");
            return -RT_ERROR;
        }
        if (rt_strcmp(argv[2], "quick") == 0)
        {
            if (sdram_data_test() && sdram_address_test())
            {
                rt_kprintf("SDRAM QUICK TEST PASS\n");
                return 0;
            }
            return -RT_ERROR;
        }
        if (rt_strcmp(argv[2], "full") == 0)
        {
            rt_kprintf("SDRAM FULL TEST: destructive 32 MiB\n");
            if (sdram_full_test())
            {
                rt_kprintf("SDRAM FULL TEST PASS\n");
                return 0;
            }
            return -RT_ERROR;
        }
    }
    rt_kprintf("Usage: sdram status | init | test quick | test full\n");
    rt_kprintf("WARNING: tests overwrite SDRAM; not allocated yet.\n");
    return -RT_EINVAL;
}
MSH_CMD_EXPORT(sdram, Initialize and test 32MiB external SDRAM);
#endif
