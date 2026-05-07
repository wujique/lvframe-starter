/**
 * device_info_page.c — 设备信息二级页面
 *
 * 功能：
 *   1. 半透明黑色背景覆盖层（40% 不透明度）
 *   2. 居中显示设备信息卡片（设备ID、名称、位置等）
 *   3. 点击页面任意位置返回上一页
 */

#include "device_info_page.h"
#include "models/device_store.h"
#include "models/device_model.h"
#include "lvframe/page_manager.h"
#include "lvframe/page.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    AppBus*            bus;
    lv_device_store_t* store;
    int                device_id;
} DeviceInfoData;

static void on_bg_clicked(lv_event_t* e)
{
    (void)e;
    lv_event_stop_bubbling(e);
    page_manager_back();
}

static void on_card_clicked(lv_event_t* e)
{
    /* 卡片区域也响应点击，但阻止冒泡到背景（若需要卡片点击也退出，可不阻止） */
    /* 需求：点击设备信息页任何地方都退出，所以这里也退出 */
    (void)e;
    lv_event_stop_bubbling(e);
    page_manager_back();
}

static void on_create(Page* page, void* params)
{
    DeviceInfoData* d = (DeviceInfoData*)params;
    /* 保存到 user_data 供 on_destroy 使用 */
    page_set_user_data(page, d);
    lv_obj_t* root = page_get_root(page);

    /* ── 全屏半透明黑色背景（40% 不透明度） ── */
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_40, LV_PART_MAIN);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root, on_bg_clicked, LV_EVENT_CLICKED, NULL);

    /* ── 信息卡片（白色背景，圆角） ── */
    lv_obj_t* card = lv_obj_create(root);
    lv_obj_set_size(card, 320, 260);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 24, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, on_card_clicked, LV_EVENT_CLICKED, NULL);

    /* ── 标题 ── */
    lv_obj_t* lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, "设备信息");
    lv_obj_set_style_text_font(lbl_title, lv_obj_get_style_text_font(root, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 0);

    /* ── 分隔线 ── */
    lv_obj_t* line = lv_obj_create(card);
    lv_obj_set_size(line, LV_PCT(100), 1);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_bg_color(line, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN);

    /* ── 读取设备信息 ── */
    char id_str[32]   = "--";
    char name_str[64] = "--";
    char type_str[32] = "--";
    char loc_str[64]  = "未设置";

    box86_light_model_t snap;
    if (box86_store_snapshot_light(d->store, d->device_id, &snap) == 0) {
        snprintf(id_str,   sizeof(id_str),   "%d", snap.base.id);
        snprintf(name_str, sizeof(name_str), "%s", snap.base.name);
        switch (snap.base.type) {
            case BOX86_DEVICE_TYPE_LIGHT:   snprintf(type_str, sizeof(type_str), "普通灯"); break;
            case BOX86_DEVICE_TYPE_CCT:     snprintf(type_str, sizeof(type_str), "色温灯"); break;
            case BOX86_DEVICE_TYPE_CURTAIN: snprintf(type_str, sizeof(type_str), "电动窗帘"); break;
            default: snprintf(type_str, sizeof(type_str), "未知"); break;
        }
    }

    /* ── 信息行辅助函数（用 flex 列布局） ── */
    lv_obj_t* rows = lv_obj_create(card);
    lv_obj_set_size(rows, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(rows, LV_ALIGN_TOP_LEFT, 0, 44);
    lv_obj_set_style_border_width(rows, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rows, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(rows, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(rows, 14, LV_PART_MAIN);
    lv_obj_clear_flag(rows, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(rows, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rows, LV_FLEX_FLOW_COLUMN);

    /* 行数据 */
    struct { const char* label; const char* value; } row_data[] = {
        { "设备 ID",   id_str   },
        { "设备名称", name_str  },
        { "设备类型", type_str  },
        { "安装位置", loc_str   },
    };
    int row_count = (int)(sizeof(row_data) / sizeof(row_data[0]));

    for (int i = 0; i < row_count; i++) {
        lv_obj_t* row = lv_obj_create(rows);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* lbl_key = lv_label_create(row);
        lv_label_set_text(lbl_key, row_data[i].label);
        lv_obj_set_style_text_color(lbl_key, lv_color_hex(0x888888), LV_PART_MAIN);
        lv_obj_set_width(lbl_key, 90);

        lv_obj_t* lbl_sep = lv_label_create(row);
        lv_label_set_text(lbl_sep, "：");
        lv_obj_set_style_text_color(lbl_sep, lv_color_hex(0x888888), LV_PART_MAIN);

        lv_obj_t* lbl_val = lv_label_create(row);
        lv_label_set_text(lbl_val, row_data[i].value);
        lv_obj_set_style_text_color(lbl_val, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_flex_grow(lbl_val, 1);
    }

    /* ── 提示文字 ── */
    lv_obj_t* lbl_hint = lv_label_create(card);
    lv_label_set_text(lbl_hint, "点击任意位置返回");
    lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_align(lbl_hint, LV_ALIGN_BOTTOM_MID, 0, 0);
}

static void on_destroy(Page* page)
{
    DeviceInfoData* d = (DeviceInfoData*)page_get_user_data(page);
    if (d) free(d);
}

Page* device_info_page_creator(void* params)
{
    DeviceInfoPageParams* p = (DeviceInfoPageParams*)params;

    DeviceInfoData* d = (DeviceInfoData*)malloc(sizeof(DeviceInfoData));
    if (!d) return NULL;
    d->bus       = p->bus;
    d->store     = p->store;
    d->device_id = p->device_id;

    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
    };
    Page* page = page_create(&lc, d);
    if (!page) { free(d); return NULL; }
    return page;
}
