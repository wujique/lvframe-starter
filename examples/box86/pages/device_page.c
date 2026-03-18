#include "device_page_internal.h"
#include <stdlib.h>

/* 各设备页构建/刷新函数声明 */
lv_obj_t* light_page_build(lv_obj_t* tile, DevicePageData* d);
void      light_page_refresh(lv_obj_t* tile, DevicePageData* d);

lv_obj_t* cct_light_page_build(lv_obj_t* tile, DevicePageData* d);
void      cct_light_page_refresh(lv_obj_t* tile, DevicePageData* d);

lv_obj_t* curtain_page_build(lv_obj_t* tile, DevicePageData* d);
void      curtain_page_refresh(lv_obj_t* tile, DevicePageData* d);

lv_obj_t* device_page_create(lv_obj_t* tile, DevicePageParams* params)
{
    DevicePageData* d = calloc(1, sizeof(DevicePageData));
    d->bus       = params->bus;
    d->store     = params->store;
    d->device_id = params->device_id;

    /* 读公共基础信息确定类型 */
    DeviceBase bases[MAX_DEVICES];
    int order[MAX_DEVICES], count = 0;
    device_store_snapshot_base(params->store, bases, order, &count);
    for (int i = 0; i < count; i++) {
        if (bases[i].id == params->device_id) { d->type = bases[i].type; break; }
    }

    lv_obj_set_user_data(tile, d);

    switch (d->type) {
    case DEVICE_TYPE_CCT:     cct_light_page_build(tile, d); break;
    case DEVICE_TYPE_CURTAIN: curtain_page_build(tile, d);   break;
    default:                  light_page_build(tile, d);     break;
    }

    device_page_refresh(tile);
    return tile;
}

void device_page_refresh(lv_obj_t* tile)
{
    DevicePageData* d = lv_obj_get_user_data(tile);
    if (!d) return;

    switch (d->type) {
    case DEVICE_TYPE_CCT:     cct_light_page_refresh(tile, d); break;
    case DEVICE_TYPE_CURTAIN: curtain_page_refresh(tile, d);   break;
    default:                  light_page_refresh(tile, d);     break;
    }
}

void device_page_destroy(lv_obj_t* tile)
{
    DevicePageData* d = lv_obj_get_user_data(tile);
    if (d) {
        free(d);
        lv_obj_set_user_data(tile, NULL);
    }
}
