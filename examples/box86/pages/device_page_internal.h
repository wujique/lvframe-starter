#ifndef DEVICE_PAGE_INTERNAL_H
#define DEVICE_PAGE_INTERNAL_H

/**
 * 设备页内部共享数据结构，仅供各设备页 .c 文件使用，不对外暴露。
 */

#include "device_page.h"

typedef struct {
    AppBus*      bus;
    DeviceStore* store;
    int          device_id;
    DeviceType   type;

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
} DevicePageData;

#endif /* DEVICE_PAGE_INTERNAL_H */
