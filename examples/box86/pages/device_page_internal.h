#ifndef DEVICE_PAGE_INTERNAL_H
#define DEVICE_PAGE_INTERNAL_H

/**
 * @file         device_page_internal.h
 * @brief        设备页内部共享数据结构及各子页面接口声明
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */

#include "device_page.h"
#include "models/model_store.h"

typedef struct {
    model_store_t* store;   /**< 模型仓库 */
    void*          model;   /**< 绑定模型指针（object）*/
    int            type;    /**< 模型类型 */

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

    /* 色温灯专用：渐变边框 + 亮区流动 */
    lv_obj_t*       cct_border_canvas;
    lv_draw_buf_t*  cct_border_buf;
    lv_timer_t*     cct_border_timer;
    int             cct_spot_pos;      /* 亮区当前周长位置（像素），-1=静止 */
    int             cct_anim_ticks;    /* 剩余动画帧数 */
} DevicePageData;

void light_page_build(lv_obj_t* root, DevicePageData* d);
void light_page_refresh(DevicePageData* d);
void light_page_destroy(DevicePageData* d);

void cct_light_page_build(lv_obj_t* root, DevicePageData* d);
void cct_light_page_refresh(DevicePageData* d);
void cct_light_page_destroy(DevicePageData* d);

void curtain_page_build(lv_obj_t* root, DevicePageData* d);
void curtain_page_refresh(DevicePageData* d);

#endif /* DEVICE_PAGE_INTERNAL_H */
