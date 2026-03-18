/**
 * platform_rk3506.c - RK3506 Linux 平台实现
 *
 * 依赖：Linux framebuffer 或 DRM/KMS
 * 用途：目标硬件 RK3506 上运行
 */

#include "../platform.h"

/* TODO: 实现 framebuffer/DRM flush 回调 */
static void rk_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    /* framebuffer 或 DRM 渲染实现 */
    lv_display_flush_ready(disp);
}

/* TODO: 实现触摸屏输入读取回调（/dev/input/eventX） */
static void rk_indev_read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{
    /* 触摸事件读取实现 */
}

void platform_init(uint32_t hor_res, uint32_t ver_res)
{
    /* 创建 LVGL 显示 */
    lv_display_t* disp = lv_display_create(hor_res, ver_res);
    lv_display_set_flush_cb(disp, rk_flush_cb);

    /* 创建 LVGL 输入设备 */
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, rk_indev_read_cb);
}

void platform_deinit(void)
{
    /* 释放 framebuffer/DRM 资源 */
}

void platform_delay_ms(uint32_t ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}
