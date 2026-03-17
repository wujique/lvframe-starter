/**
 * platform_rtos.c - RTOS 平台实现（预留）
 *
 * 用途：未来移植到 RTOS 平台时在此实现
 */

#include "../platform.h"

void platform_init(void)
{
    /* TODO: RTOS 平台初始化 */
}

void platform_deinit(void)
{
    /* TODO: RTOS 平台资源释放 */
}

void platform_delay_ms(uint32_t ms)
{
    /* TODO: RTOS 延时实现，如 vTaskDelay */
    (void)ms;
}
