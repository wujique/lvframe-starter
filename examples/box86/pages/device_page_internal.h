#ifndef DEVICE_PAGE_INTERNAL_H
#define DEVICE_PAGE_INTERNAL_H

/**
 * 设备页内部共享数据结构，仅供各设备页 .c 文件使用，不对外暴露。
 */

#include "device_page.h"
#include "models/device_store.h"

typedef struct {
    AppBus*      bus;
    lv_device_store_t* store;
    int          device_id;
    int                type;

    lv_obj_t*    lbl_name;
    lv_obj_t*    lbl_status;

    /* 普通灯 / 色温灯 */
    lv_obj_t*    btn_toggle;

    /* 色温灯专用 */
    lv_obj_t*    slider_cct;
    lv_obj_t*    lbl_cct;

    /* 电动窗帘专用 */
    lv_obj_t*    btn_open;
    lv_obj_t*    btn_close;
    lv_obj_t*    btn_stop;

    /* 普通灯专用：彩虹边框 */
    lv_obj_t*       border_canvas;   /* 覆盖整个页面的透明 canvas，只画四边 */
    lv_draw_buf_t*  border_buf;      /* canvas 的像素缓冲 */
    lv_timer_t*     border_timer;    /* 流水灯动画定时器 */
    int             border_hue_off;  /* 当前色相偏移 (0~359) */
    int             border_anim_ticks; /* 剩余动画帧数，0=静止 */
} DevicePageData;

/* 各设备页构建/刷新函数声明 */
void light_page_build(lv_obj_t* root, DevicePageData* d);
void light_page_refresh(DevicePageData* d);
void light_page_destroy(DevicePageData* d);

void cct_light_page_build(lv_obj_t* root, DevicePageData* d);
void cct_light_page_refresh(DevicePageData* d);

void curtain_page_build(lv_obj_t* root, DevicePageData* d);
void curtain_page_refresh(DevicePageData* d);

#endif /* DEVICE_PAGE_INTERNAL_H */
