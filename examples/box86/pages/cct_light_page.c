/**
 * cct_light_page.c — 色温灯设备页
 *
 * 边框效果（参考 ref/pic_1.png）：
 *   1. 静态：四周 20px 渐变色带，HSV 色相按周长分布，内边缘渐变到透明
 *   2. 动画：点击开/关时，一个高亮"亮区"光斑沿四周顺时针流动一圈（约 800ms）
 */

#include "device_page_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ── 边框参数 ── */
#define CCT_BORDER_W      20          /* 色带宽度（像素） */
#define CCT_ANIM_TOTAL_MS 2000        /* 亮区绕行一圈时长（毫秒） */
#define CCT_ANIM_TICK_MS  16          /* 定时器间隔（毫秒） */
#define CCT_ANIM_TICKS    (CCT_ANIM_TOTAL_MS / CCT_ANIM_TICK_MS)
#define CCT_SPOT_WIDTH    200         /* 亮区光斑宽度（周长像素） */

/* ── 工具：按周长位置计算色相（0~359） ── */
static int cct_hue_at(int pos, int perimeter)
{
    int h = (int)(360LL * pos / perimeter);
    return ((h % 360) + 360) % 360;
}

/* ── 工具：高斯形亮区强度（0~255），dist 为到光斑中心的距离
 * sigma = CCT_SPOT_WIDTH/2.5，使光斑中间饱满、两端平滑渐暗 ── */
static int spot_intensity(int dist)
{
    if (dist < 0) dist = -dist;
    float sigma = CCT_SPOT_WIDTH / 2.5f;
    float v = expf(-(float)(dist * dist) / (2.0f * sigma * sigma));
    int iv = (int)(v * 255.0f + 0.5f);
    return iv > 255 ? 255 : iv;
}

/*
 * ── 设置 canvas 某像素的 ARGB8888 值（带 alpha）──
 * LVGL 9 的 lv_canvas_set_px 不直接支持 alpha 通道叠加，
 * 这里直接操作 draw_buf 的原始内存（ARGB8888 小端：B G R A 顺序）。
 */
static void canvas_set_pixel_argb(lv_draw_buf_t* buf, int32_t w, int32_t x, int32_t y,
                                   uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (!buf || !buf->data) return;
    /* ARGB8888: 每像素 4 字节，内存布局 B G R A（little-endian） */
    uint32_t stride = buf->header.stride; /* bytes per row */
    uint8_t* p = (uint8_t*)buf->data + (uint32_t)y * stride + (uint32_t)x * 4;
    p[0] = b;
    p[1] = g;
    p[2] = r;
    p[3] = a;
}

/* ── 绘制色带（带内边缘渐变 + 可选亮区叠加） ── */
static void cct_draw_border(DevicePageData* d)
{
    lv_obj_t*      canvas = d->cct_border_canvas;
    lv_draw_buf_t* buf    = d->cct_border_buf;
    if (!canvas || !buf) return;

    int32_t W = lv_obj_get_width(canvas);
    int32_t H = lv_obj_get_height(canvas);
    if (W <= 0 || H <= 0) return;

    int bw = CCT_BORDER_W;

    /* 先清空为全透明 */
    memset(buf->data, 0, (size_t)buf->header.stride * (uint32_t)H);

    /*
     * 周长：沿四边中心线，顺时针
     *   上边：W 个像素
     *   右边：H 个像素
     *   下边：W 个像素
     *   左边：H 个像素
     * （与普通灯保持一致的简化处理）
     */
    int perimeter = 2 * (W + H);
    int spot_pos  = d->cct_spot_pos; /* -1 表示无动画 */

    /* 上边 */
    for (int x = 0; x < W; x++) {
        int seg_pos = x; /* 本段在周长上的位置 */
        int hue = cct_hue_at(seg_pos, perimeter);
        lv_color_t c = lv_color_hsv_to_rgb((uint16_t)hue, 200, 255);

        for (int t = 0; t < bw; t++) {
            /* t=0 外边缘（顶），t=bw-1 内边缘（靠页面中心）
             * 内边缘透明度渐变到 0 */
            uint8_t base_a = (uint8_t)(255 - (int)(255 * t / (bw - 1)));

            /* 叠加亮区 */
            uint8_t a = base_a;
            if (spot_pos >= 0) {
                int dist = seg_pos - spot_pos;
                /* 环绕处理 */
                if (dist > perimeter / 2)  dist -= perimeter;
                if (dist < -perimeter / 2) dist += perimeter;
                int si = spot_intensity(dist);
                /* 亮区使 alpha 和亮度都提升 */
                int new_a = base_a + (255 - base_a) * si / 255;
                a = (uint8_t)(new_a > 255 ? 255 : new_a);
                /* 颜色向白色偏移 */
                int nr = c.red   + (255 - c.red)   * si / 255;
                int ng = c.green + (255 - c.green) * si / 255;
                int nb = c.blue  + (255 - c.blue)  * si / 255;
                canvas_set_pixel_argb(buf, W, x, t,
                    (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, a);
                continue;
            }
            canvas_set_pixel_argb(buf, W, x, t, c.red, c.green, c.blue, a);
        }
    }

    /* 右边 */
    for (int y = 0; y < H; y++) {
        int seg_pos = W + y;
        int hue = cct_hue_at(seg_pos, perimeter);
        lv_color_t c = lv_color_hsv_to_rgb((uint16_t)hue, 200, 255);

        for (int t = 0; t < bw; t++) {
            /* t=0 外边缘（右侧），t=bw-1 内边缘 */
            uint8_t base_a = (uint8_t)(255 - (int)(255 * t / (bw - 1)));
            int px = W - 1 - t;
            uint8_t a = base_a;
            if (spot_pos >= 0) {
                int dist = seg_pos - spot_pos;
                if (dist > perimeter / 2)  dist -= perimeter;
                if (dist < -perimeter / 2) dist += perimeter;
                int si = spot_intensity(dist);
                int new_a = base_a + (255 - base_a) * si / 255;
                a = (uint8_t)(new_a > 255 ? 255 : new_a);
                int nr = c.red   + (255 - c.red)   * si / 255;
                int ng = c.green + (255 - c.green) * si / 255;
                int nb = c.blue  + (255 - c.blue)  * si / 255;
                canvas_set_pixel_argb(buf, W, px, y,
                    (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, a);
                continue;
            }
            canvas_set_pixel_argb(buf, W, px, y, c.red, c.green, c.blue, a);
        }
    }

    /* 下边（从右到左，顺时针） */
    for (int x = W - 1; x >= 0; x--) {
        int seg_pos = W + H + (W - 1 - x);
        int hue = cct_hue_at(seg_pos, perimeter);
        lv_color_t c = lv_color_hsv_to_rgb((uint16_t)hue, 200, 255);

        for (int t = 0; t < bw; t++) {
            /* t=0 外边缘（底部），t=bw-1 内边缘 */
            uint8_t base_a = (uint8_t)(255 - (int)(255 * t / (bw - 1)));
            int py = H - 1 - t;
            uint8_t a = base_a;
            if (spot_pos >= 0) {
                int dist = seg_pos - spot_pos;
                if (dist > perimeter / 2)  dist -= perimeter;
                if (dist < -perimeter / 2) dist += perimeter;
                int si = spot_intensity(dist);
                int new_a = base_a + (255 - base_a) * si / 255;
                a = (uint8_t)(new_a > 255 ? 255 : new_a);
                int nr = c.red   + (255 - c.red)   * si / 255;
                int ng = c.green + (255 - c.green) * si / 255;
                int nb = c.blue  + (255 - c.blue)  * si / 255;
                canvas_set_pixel_argb(buf, W, x, py,
                    (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, a);
                continue;
            }
            canvas_set_pixel_argb(buf, W, x, py, c.red, c.green, c.blue, a);
        }
    }

    /* 左边（从下到上，顺时针） */
    for (int y = H - 1; y >= 0; y--) {
        int seg_pos = W + H + W + (H - 1 - y);
        int hue = cct_hue_at(seg_pos, perimeter);
        lv_color_t c = lv_color_hsv_to_rgb((uint16_t)hue, 200, 255);

        for (int t = 0; t < bw; t++) {
            /* t=0 外边缘（左侧），t=bw-1 内边缘 */
            uint8_t base_a = (uint8_t)(255 - (int)(255 * t / (bw - 1)));
            int px = t;
            uint8_t a = base_a;
            if (spot_pos >= 0) {
                int dist = seg_pos - spot_pos;
                if (dist > perimeter / 2)  dist -= perimeter;
                if (dist < -perimeter / 2) dist += perimeter;
                int si = spot_intensity(dist);
                int new_a = base_a + (255 - base_a) * si / 255;
                a = (uint8_t)(new_a > 255 ? 255 : new_a);
                int nr = c.red   + (255 - c.red)   * si / 255;
                int ng = c.green + (255 - c.green) * si / 255;
                int nb = c.blue  + (255 - c.blue)  * si / 255;
                canvas_set_pixel_argb(buf, W, px, y,
                    (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, a);
                continue;
            }
            canvas_set_pixel_argb(buf, W, px, y, c.red, c.green, c.blue, a);
        }
    }

    /* 通知 LVGL canvas 内容已更新 */
    lv_obj_invalidate(canvas);
}

/* ── 动画定时器回调 ── */
static void cct_border_timer_cb(lv_timer_t* timer)
{
    DevicePageData* d = lv_timer_get_user_data(timer);
    if (!d || !d->cct_border_canvas) return;

    if (d->cct_anim_ticks <= 0) return;

    int32_t W = lv_obj_get_width(d->cct_border_canvas);
    int32_t H = lv_obj_get_height(d->cct_border_canvas);
    int perimeter = 2 * (W + H);
    int step = perimeter / CCT_ANIM_TICKS + 1;

    d->cct_spot_pos = (d->cct_spot_pos + step) % perimeter;
    d->cct_anim_ticks--;

    if (d->cct_anim_ticks <= 0) {
        d->cct_spot_pos = -1; /* 动画结束，隐藏光斑 */
    }

    cct_draw_border(d);
}

/* ── 触发亮区流动动画 ── */
static void cct_start_anim(DevicePageData* d)
{
    d->cct_spot_pos   = 0;
    d->cct_anim_ticks = CCT_ANIM_TICKS;
}

/* ── 开关按钮回调 ── */
static void on_toggle(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    box86_cct_light_model_t snap;
    if (box86_store_snapshot_cct(d->store, d->device_id, &snap) < 0) return;

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = snap.onoffsta ? 0 : 1;
    strncpy(msg.field, "onoffsta", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);

    cct_start_anim(d);
}

static void on_slider_cct(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    int val = (int)lv_slider_get_value(d->slider_cct);

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = val;
    strncpy(msg.field, "color_temp", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);

    char buf[16];
    snprintf(buf, sizeof(buf), "%dK", val);
    lv_label_set_text(d->lbl_cct, buf);
}

void cct_light_page_build(lv_obj_t* root, DevicePageData* d)
{
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0xE3F2FD), LV_PART_MAIN); /* 浅蓝，与原始一致 */
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);

    d->lbl_name = lv_label_create(root);
    lv_label_set_text(d->lbl_name, "...");
    lv_obj_align(d->lbl_name, LV_ALIGN_TOP_MID, 0, 20);

    d->btn_toggle = lv_button_create(root);
    lv_obj_set_size(d->btn_toggle, 120, 50);
    lv_obj_align(d->btn_toggle, LV_ALIGN_CENTER, 0, -40);
    lv_obj_t* lbl = lv_label_create(d->btn_toggle);
    lv_label_set_text(lbl, "On/Off");
    lv_obj_center(lbl);
    lv_obj_add_event_cb(d->btn_toggle, on_toggle, LV_EVENT_CLICKED, d);

    d->lbl_status = lv_label_create(root);
    lv_label_set_text(d->lbl_status, "---");
    lv_obj_align(d->lbl_status, LV_ALIGN_CENTER, 0, 20);

    d->slider_cct = lv_slider_create(root);
    lv_slider_set_range(d->slider_cct, 2700, 6500);
    lv_slider_set_value(d->slider_cct, 4000, LV_ANIM_OFF);
    lv_obj_set_width(d->slider_cct, 200);
    lv_obj_align(d->slider_cct, LV_ALIGN_CENTER, 0, 80);
    lv_obj_add_event_cb(d->slider_cct, on_slider_cct, LV_EVENT_VALUE_CHANGED, d);

    d->lbl_cct = lv_label_create(root);
    lv_label_set_text(d->lbl_cct, "4000K");
    lv_obj_align(d->lbl_cct, LV_ALIGN_CENTER, 0, 115);

    /* ── 边框 canvas ── */
    int32_t sw = 480, sh = 480;
    d->cct_border_buf = lv_draw_buf_create(sw, sh, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
    d->cct_border_canvas = lv_canvas_create(root);
    lv_canvas_set_draw_buf(d->cct_border_canvas, d->cct_border_buf);
    lv_obj_set_size(d->cct_border_canvas, sw, sh);
    lv_obj_align(d->cct_border_canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_flag(d->cct_border_canvas, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(d->cct_border_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(d->cct_border_canvas, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_move_foreground(d->cct_border_canvas);

    d->cct_spot_pos   = -1; /* 无动画 */
    d->cct_anim_ticks = 0;

    /* 初始静态绘制 */
    cct_draw_border(d);

    /* 创建常驻定时器 */
    d->cct_border_timer = lv_timer_create(cct_border_timer_cb, CCT_ANIM_TICK_MS, d);
}

void cct_light_page_refresh(DevicePageData* d)
{
    box86_cct_light_model_t snap;
    if (box86_store_snapshot_cct(d->store, d->device_id, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    lv_label_set_text(d->lbl_status, snap.onoffsta ? "ON" : "OFF");
    lv_slider_set_value(d->slider_cct, snap.color_temp, LV_ANIM_OFF);
    char buf[16];
    snprintf(buf, sizeof(buf), "%dK", snap.color_temp);
    lv_label_set_text(d->lbl_cct, buf);
}

void cct_light_page_destroy(DevicePageData* d)
{
    if (d->cct_border_timer) {
        lv_timer_delete(d->cct_border_timer);
        d->cct_border_timer = NULL;
    }
    if (d->cct_border_buf) {
        lv_draw_buf_destroy(d->cct_border_buf);
        d->cct_border_buf = NULL;
    }
}
