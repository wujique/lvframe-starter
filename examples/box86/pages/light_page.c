/**
 * light_page.c — 普通灯设备页
 *
 * 功能：
 *   1. 静态彩虹边框：页面四周 10px，颜色按周长位置映射 HSV 色相（0→360）
 *   2. 流水灯动画：点击开/关按钮时，彩虹色相绕页面旋转一圈（约 800ms）
 */

#include "device_page_internal.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ── 边框参数 ── */
#define BORDER_W      20          /* 边框宽度（像素） */
#define ANIM_TOTAL_MS 800         /* 流水灯一圈时长（毫秒） */
#define ANIM_TICK_MS  16          /* 定时器间隔（毫秒，约 60fps） */
#define ANIM_TICKS    (ANIM_TOTAL_MS / ANIM_TICK_MS)   /* 总帧数 ≈ 50 */

/* ── HSV → lv_color_t（LVGL 9 内置 lv_color_hsv_to_rgb，但此处手写以免依赖） ── */
static lv_color_t hsv_to_color(int h, int s, int v)
{
    /* h: 0-359, s: 0-255, v: 0-255 */
    lv_color_hsv_t hsv = {(uint16_t)h, (uint8_t)s, (uint8_t)v};
    return lv_color_hsv_to_rgb(hsv.h, hsv.s, hsv.v);
}

/* ── 计算边框上某个"周长位置"对应的色相 ── */
/* perimeter_pos: 0 ~ (perimeter-1)，顺时针从左上角开始
 * hue_offset: 动画偏移量
 * 返回 0~359
 */
static int border_hue(int perimeter_pos, int perimeter, int hue_offset)
{
    int h = (int)(360LL * perimeter_pos / perimeter) + hue_offset;
    return ((h % 360) + 360) % 360;
}

/* ── 绘制边框到 canvas ── */
static void draw_border(DevicePageData* d)
{
    lv_obj_t* canvas = d->border_canvas;
    if (!canvas) return;

    int32_t w = lv_obj_get_width(canvas);
    int32_t h = lv_obj_get_height(canvas);
    if (w <= 0 || h <= 0) return;

    int bw = BORDER_W;
    int perimeter = 2 * (w + h) - 4 * bw; /* 边框中心线周长（近似） */
    if (perimeter <= 0) return;

    int hue_off = d->border_hue_off;

    /* 清空（全透明） */
    lv_canvas_fill_bg(canvas, lv_color_make(0, 0, 0), LV_OPA_TRANSP);

    /* 按顺时针方向绘制四边：上、右、下（反向）、左（反向） */
    /* 每边的"周长起始 pos" */
    int pos = 0;

    /* 上边：从 (0, 0) → (w-1, 0)，水平，bw 行高 */
    for (int x = 0; x < w; x++, pos++) {
        int hue = border_hue(pos, perimeter, hue_off);
        lv_color_t c = hsv_to_color(hue, 255, 255);
        for (int y = 0; y < bw; y++) {
            lv_canvas_set_px(canvas, x, y, c, LV_OPA_COVER);
        }
    }

    /* 右边：从 (w-1, bw) → (w-1, h-1)，垂直，bw 列宽 */
    for (int y = bw; y < h; y++, pos++) {
        int hue = border_hue(pos, perimeter, hue_off);
        lv_color_t c = hsv_to_color(hue, 255, 255);
        for (int x = w - bw; x < w; x++) {
            lv_canvas_set_px(canvas, x, y, c, LV_OPA_COVER);
        }
    }

    /* 下边：从 (w-1-bw, h-1) → (0, h-1)，反向 */
    for (int x = w - bw - 1; x >= 0; x--, pos++) {
        int hue = border_hue(pos, perimeter, hue_off);
        lv_color_t c = hsv_to_color(hue, 255, 255);
        for (int y = h - bw; y < h; y++) {
            lv_canvas_set_px(canvas, x, y, c, LV_OPA_COVER);
        }
    }

    /* 左边：从 (0, h-1-bw) → (0, bw)，反向 */
    for (int y = h - bw - 1; y >= bw; y--, pos++) {
        int hue = border_hue(pos, perimeter, hue_off);
        lv_color_t c = hsv_to_color(hue, 255, 255);
        for (int x = 0; x < bw; x++) {
            lv_canvas_set_px(canvas, x, y, c, LV_OPA_COVER);
        }
    }
}

/* ── 动画定时器回调 ── */
static void border_anim_timer_cb(lv_timer_t* timer)
{
    DevicePageData* d = lv_timer_get_user_data(timer);
    if (!d || !d->border_canvas) return;

    if (d->border_anim_ticks > 0) {
        /* 每帧旋转 360/ANIM_TICKS 度 */
        d->border_hue_off = (d->border_hue_off + 360 / ANIM_TICKS + 1) % 360;
        d->border_anim_ticks--;
        draw_border(d);
    }
    /* 动画结束后保持最后状态，定时器继续存在（静止，不绘制） */
}

/* ── 触发流水灯动画 ── */
static void start_border_anim(DevicePageData* d)
{
    d->border_anim_ticks = ANIM_TICKS;
}

/* ── 开关按钮回调 ── */
static void on_toggle(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    box86_light_model_t snap;
    if (box86_store_snapshot_light(d->store, d->device_id, &snap) < 0) return;

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = snap.onoffsta ? 0 : 1;
    strncpy(msg.field, "onoffsta", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);

    /* 触发流水灯旋转 */
    start_border_anim(d);
}

/* ── 构建页面 ── */
void light_page_build(lv_obj_t* root, DevicePageData* d)
{
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0xFFFDE7), LV_PART_MAIN); /* 暖黄 */
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);

    d->lbl_name = lv_label_create(root);
    lv_label_set_text(d->lbl_name, "...");
    lv_obj_align(d->lbl_name, LV_ALIGN_TOP_MID, 0, 20);

    d->btn_toggle = lv_button_create(root);
    lv_obj_set_size(d->btn_toggle, 120, 50);
    lv_obj_align(d->btn_toggle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t* lbl = lv_label_create(d->btn_toggle);
    lv_label_set_text(lbl, "开/关");
    lv_obj_center(lbl);
    lv_obj_add_event_cb(d->btn_toggle, on_toggle, LV_EVENT_CLICKED, d);

    d->lbl_status = lv_label_create(root);
    lv_label_set_text(d->lbl_status, "---");
    lv_obj_align(d->lbl_status, LV_ALIGN_CENTER, 0, 60);

    /* 汉字显示测试 */
    lv_obj_t* lbl_test = lv_label_create(root);
    lv_label_set_text(lbl_test, "汉字测试：普通灯控制面板\n亮度调节 · 开关状态 · 场景模式");
    lv_obj_set_width(lbl_test, LV_PCT(90));
    lv_label_set_long_mode(lbl_test, LV_LABEL_LONG_WRAP);
    lv_obj_align(lbl_test, LV_ALIGN_BOTTOM_MID, 0, -20);

    /* ── 彩虹边框 canvas ── */
    /* 等 root 布局确定后再创建；此处用屏幕尺寸 480×480 */
    int32_t sw = 480, sh = 480;

    d->border_buf = lv_draw_buf_create(sw, sh, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
    d->border_canvas = lv_canvas_create(root);
    lv_canvas_set_draw_buf(d->border_canvas, d->border_buf);
    lv_obj_set_size(d->border_canvas, sw, sh);
    lv_obj_align(d->border_canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    /* 置于所有子控件上层，但自身不拦截事件 */
    lv_obj_add_flag(d->border_canvas, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(d->border_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(d->border_canvas, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_move_foreground(d->border_canvas);

    d->border_hue_off   = 0;
    d->border_anim_ticks = 0;

    /* 初始绘制静态彩虹边框 */
    draw_border(d);

    /* 创建定时器驱动动画（一直存在，静止时不绘制） */
    d->border_timer = lv_timer_create(border_anim_timer_cb, ANIM_TICK_MS, d);
}

/* ── 刷新页面数据 ── */
void light_page_refresh(DevicePageData* d)
{
    box86_light_model_t snap;
    if (box86_store_snapshot_light(d->store, d->device_id, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    lv_label_set_text(d->lbl_status, snap.onoffsta ? "已开启" : "已关闭");
}

/* ── 销毁时清理资源 ── */
void light_page_destroy(DevicePageData* d)
{
    if (d->border_timer) {
        lv_timer_delete(d->border_timer);
        d->border_timer = NULL;
    }
    if (d->border_buf) {
        lv_draw_buf_destroy(d->border_buf);
        d->border_buf = NULL;
    }
}
