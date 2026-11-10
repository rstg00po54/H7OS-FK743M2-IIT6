/* H7OS FK743M2 SDRAM; 32MiB isolated from RTOS heap. */
#ifndef H7OS_SDRAM_H
#define H7OS_SDRAM_H
#include <rtthread.h>
#define H7OS_SDRAM_BASE 0xC0000000UL
#define H7OS_SDRAM_SIZE (32UL * 1024UL * 1024UL)
rt_bool_t h7os_sdram_is_ready(void);
#endif
