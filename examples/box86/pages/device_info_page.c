/**
 * device_info_page.c — 设备信息覆盖层
 *
 * 实现为覆盖层（overlay），直接附加在设备页 root 上，
 * 而非独立的 Page，这样半透明背景可以透出底层设备页内容。
 */

#include "device_info_page.h"
#include "models/device_store.h"
#include "models/device_model.h"
#include <string.h>
#include <stdio.h>

/* ── 点击覆盖层任意位置关闭 ── */
static void on_overlay_clicked(lv_event_t* e)
{
    lv_obj_t* overlay = (lv_obj_t*)lv_event_get_user_data(e);
    lv_event_stop_bubbling(e);
    lv_obj_delete(overlay);
}

void device_info_overlay_open(lv_obj_t* parent, lv_device_store_t* store, int device_id)
{
    /* ── 全屏半透明覆盖层，附加在 parent（设备页 root）上 ── */
    lv_obj_t* overlay = lv_obj_create(parent);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(overlay, on_overlay_clicked, LV_EVENT_CLICKED, overlay);
    lv_obj_move_foreground(overlay);

    /* ── 信息卡片（白色背景，圆角，不透明） ── */
    lv_obj_t* card = lv_obj_create(overlay);
    lv_obj_set_size(card, 320, 280);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 24, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    /* 卡片点击也关闭（需求：点击任何地方都退出） */
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, on_overlay_clicked, LV_EVENT_CLICKED, overlay);

    /* ── 标题 ── */
    lv_obj_t* lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, "设备信息");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 0);

    /* ── 分隔线 ── */
    lv_obj_t* line = lv_obj_create(card);
    lv_obj_set_size(line, LV_PCT(100), 1);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 30);
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
    if (box86_store_snapshot_light(store, device_id, &snap) == 0) {
        snprintf(id_str,   sizeof(id_str),   "%d", snap.base.id);
        snprintf(name_str, sizeof(name_str), "%s", snap.base.name);
        switch (snap.base.type) {
            case BOX86_DEVICE_TYPE_LIGHT:   snprintf(type_str, sizeof(type_str), "普通灯");   break;
            case BOX86_DEVICE_TYPE_CCT:     snprintf(type_str, sizeof(type_str), "色温灯");   break;
            case BOX86_DEVICE_TYPE_CURTAIN: snprintf(type_str, sizeof(type_str), "电动窗帘"); break;
            default: snprintf(type_str, sizeof(type_str), "未知"); break;
        }
    }

    /* ── 信息行容器（flex 列布局） ── */
    lv_obj_t* rows = lv_obj_create(card);
    lv_obj_set_size(rows, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(rows, LV_ALIGN_TOP_LEFT, 0, 42);
    lv_obj_set_style_border_width(rows, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rows, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(rows, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(rows, 16, LV_PART_MAIN);
    lv_obj_clear_flag(rows, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(rows, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rows, LV_FLEX_FLOW_COLUMN);

    struct { const char* label; const char* value; } row_data[] = {
        { "设备 ID",   id_str   },
        { "设备名称",  name_str },
        { "设备类型",  type_str },
        { "安装位置",  loc_str  },
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
