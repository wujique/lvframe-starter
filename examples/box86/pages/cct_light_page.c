/**
 * @file         cct_light_page.c
 * @brief        色温灯设备页：渐变彩虹边框 + 亮区流动动画 + 开关/色温控制
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "device_page_internal.h"
#include "msg.h"
#include "slots.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ── 边框参数 ── */
#define CCT_BORDER_W      20          /* 色带宽度（像素） */
#define CCT_ANIM_TOTAL_MS 2000        /* 亮区绕行一圈时长（毫秒） */
#define CCT_ANIM_TICK_MS  20          /* 定时器间隔（毫秒，50fps） */
#define CCT_ANIM_TICKS    (CCT_ANIM_TOTAL_MS / CCT_ANIM_TICK_MS)
#define CCT_SPOT_WIDTH    200         /* 亮区光斑宽度（周长像素） */

/* ── 工具：高斯形亮区强度（0~255），dist 为到光斑中心的距离
 * sigma = CCT_SPOT_WIDTH/2.5，使光斑中间饱满、两端平滑渐暗 ── */
/**
 * @brief        计算高斯形亮区强度（0~255），dist 为到光斑中心的距离
 *
 * @param        dist                 到光斑中心的周长距离（像素）
 * @return       int 亮度强度 (0~255)
 */
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
/**
 * @brief        直接写入 ARGB8888 draw_buf 某像素的颜色和透明度
 *
 * @param        buf                  draw_buf 指针
 * @param        w                    画布宽度（像素）
 * @param        x                    像素 x 坐标
 * @param        y                    像素 y 坐标
 * @param        r                    红色分量
 * @param        g                    绿色分量
 * @param        b                    蓝色分量
 * @param        a                    alpha 分量
 * @return       void
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

/* ── 绘制色带（按像素扫描，角落自然衔接） ──
 *
 * 对每个位于边框区域内的像素 (x, y)：
 *   1. 计算到四条边的距离 d_top/d_bottom/d_left/d_right，取最小值 d_min
 *      d_min=0 → 外边缘（不透明），d_min=bw → 内边缘（透明）
 *   2. 色相按"最近边"的周长位置取值，角落处两边均等权重插值
 *   3. 叠加亮区光斑（若动画进行中）
 */
/**
 * @brief        绘制色温灯渐变彩虹边框到 canvas（含亮区光斑叠加）
 *
 * @param        d                    设备页共享数据（含 cct_border_canvas/cct_spot_pos）
 * @return       void
 */
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
     * 顺时针周长定义（与动画 spot_pos 对应）：
     *   上边  0         → W-1       (seg_pos = x)
     *   右边  W         → W+H-1     (seg_pos = W + y)
     *   下边  W+H       → 2W+H-1    (seg_pos = W+H + (W-1-x))
     *   左边  2W+H      → 2W+2H-1   (seg_pos = 2W+H + (H-1-y))
     */
    int perimeter = 2 * (W + H);
    int spot_pos  = d->cct_spot_pos;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            /* 到四条外边缘的距离（0 = 外边缘） */
            int d_top    = y;
            int d_bottom = H - 1 - y;
            int d_left   = x;
            int d_right  = W - 1 - x;

            /* 只处理边框区域内的像素 */
            int d_min = d_top;
            if (d_bottom < d_min) d_min = d_bottom;
            if (d_left   < d_min) d_min = d_left;
            if (d_right  < d_min) d_min = d_right;
            if (d_min >= bw) continue; /* 内部区域跳过 */

            /* 基础 alpha：外边缘=255，内边缘=0，线性渐变 */
            uint8_t base_a = (uint8_t)(255 - (int)(255 * d_min / (bw - 1)));

            /* 周长位置：取距离最小的那条边
             * 角落时两条边 d_min 相等，做线性插值 */
            int seg_top    = x;                   /* 上边 */
            int seg_right  = W + y;               /* 右边 */
            int seg_bottom = W + H + (W - 1 - x); /* 下边 */
            int seg_left   = 2*W + H + (H - 1 - y); /* 左边 */

            /* 权重：距离越小权重越大，用 1/(d+0.5) 归一化 */
            float w_top    = (d_top    < bw) ? 1.0f / (d_top    + 0.5f) : 0.0f;
            float w_right  = (d_right  < bw) ? 1.0f / (d_right  + 0.5f) : 0.0f;
            float w_bottom = (d_bottom < bw) ? 1.0f / (d_bottom + 0.5f) : 0.0f;
            float w_left   = (d_left   < bw) ? 1.0f / (d_left   + 0.5f) : 0.0f;
            float w_sum = w_top + w_right + w_bottom + w_left;
            if (w_sum < 1e-6f) continue;

            /* 加权平均色相（HSV 色相环形插值，用向量法）*/
            float sx = 0.0f, sy = 0.0f;
            float pi2 = 6.28318530718f;
            sx += w_top    * cosf(pi2 * seg_top    / perimeter);
            sy += w_top    * sinf(pi2 * seg_top    / perimeter);
            sx += w_right  * cosf(pi2 * seg_right  / perimeter);
            sy += w_right  * sinf(pi2 * seg_right  / perimeter);
            sx += w_bottom * cosf(pi2 * seg_bottom / perimeter);
            sy += w_bottom * sinf(pi2 * seg_bottom / perimeter);
            sx += w_left   * cosf(pi2 * seg_left   / perimeter);
            sy += w_left   * sinf(pi2 * seg_left   / perimeter);
            float angle = atan2f(sy / w_sum, sx / w_sum);
            if (angle < 0) angle += pi2;
            int hue = (int)(angle / pi2 * 360.0f + 0.5f) % 360;

            lv_color_t c = lv_color_hsv_to_rgb((uint16_t)hue, 200, 255);

            /* 周长代表位置：取权重最大的那条边用于光斑距离计算 */
            int seg_dominant = seg_top;
            float w_dominant = w_top;
            if (w_right  > w_dominant) { w_dominant = w_right;  seg_dominant = seg_right; }
            if (w_bottom > w_dominant) { w_dominant = w_bottom; seg_dominant = seg_bottom; }
            if (w_left   > w_dominant) {                        seg_dominant = seg_left; }

            if (spot_pos >= 0) {
                int dist = seg_dominant - spot_pos;
                if (dist > perimeter / 2)  dist -= perimeter;
                if (dist < -perimeter / 2) dist += perimeter;
                int si = spot_intensity(dist);
                /* alpha 保持 base_a 渐变不变，只将颜色向白色偏移 */
                int nr = (int)c.red   + (255 - (int)c.red)   * si / 255;
                int ng = (int)c.green + (255 - (int)c.green) * si / 255;
                int nb = (int)c.blue  + (255 - (int)c.blue)  * si / 255;
                canvas_set_pixel_argb(buf, W, x, y,
                    (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, base_a);
                continue;
            }
            canvas_set_pixel_argb(buf, W, x, y, c.red, c.green, c.blue, base_a);
        }
    }

    /* 通知 LVGL canvas 内容已更新 */
    lv_obj_invalidate(canvas);
}

/* ── 动画定时器回调 ── */
/**
 * @brief        色温灯边框动画定时器回调，推进亮区位置并重绘
 *
 * @param        timer                LVGL 定时器句柄
 * @return       void
 */
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
/**
 * @brief        触发亮区流动动画（重置位置和帧计数器）
 *
 * @param        d                    设备页共享数据
 * @return       void
 */
static void cct_start_anim(DevicePageData* d)
{
    d->cct_spot_pos   = 0;
    d->cct_anim_ticks = CCT_ANIM_TICKS;
}

/* ── 开关按钮回调 ── */
/**
 * @brief        开关按钮点击回调，发送 set_prop 消息并触发亮区动画
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
static void on_toggle(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    box86_cct_light_model_t snap;
    if (box86_store_snapshot_cct(d->store, d->model, &snap) < 0) return;

    lv_slot_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.signal = MSG_UI_SET_PROP;
    msg.object = d->model;
    msg.arg0   = snap.onoffsta ? 0 : 1;
    strncpy(msg.field, "onoffsta", sizeof(msg.field) - 1);
    lv_slot_send(&g_dev_slot, &msg);

    cct_start_anim(d);
}

/**
 * @brief        色温滑块值变化回调，发送 set_prop 消息并更新标签
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
static void on_slider_cct(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    int val = (int)lv_slider_get_value(d->slider_cct);

    lv_slot_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.signal = MSG_UI_SET_PROP;
    msg.object = d->model;
    msg.arg0   = val;
    strncpy(msg.field, "color_temp", sizeof(msg.field) - 1);
    lv_slot_send(&g_dev_slot, &msg);

    char buf[16];
    snprintf(buf, sizeof(buf), "%dK", val);
    lv_label_set_text(d->lbl_cct, buf);
}

/**
 * @brief        构建色温灯设备页 UI 及渐变边框 canvas
 *
 * @param        root                 页面根容器
 * @param        d                    设备页共享数据
 * @return       void
 */
void cct_light_page_build(lv_obj_t* root, DevicePageData* d)
{
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0x0A0A10), LV_PART_MAIN); /* 深黑，贴近参考图背景 */
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(root, lv_color_white(), LV_PART_MAIN);

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

/**
 * @brief        刷新色温灯设备页数据（名称/开关状态/色温值）
 *
 * @param        d                    设备页共享数据
 * @return       void
 */
void cct_light_page_refresh(DevicePageData* d)
{
    box86_cct_light_model_t snap;
    if (box86_store_snapshot_cct(d->store, d->model, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    lv_label_set_text(d->lbl_status, snap.onoffsta ? "ON" : "OFF");
    lv_slider_set_value(d->slider_cct, snap.color_temp, LV_ANIM_OFF);
    char buf[16];
    snprintf(buf, sizeof(buf), "%dK", snap.color_temp);
    lv_label_set_text(d->lbl_cct, buf);
}

/**
 * @brief        销毁色温灯设备页，释放定时器和绘图缓冲
 *
 * @param        d                    设备页共享数据
 * @return       void
 */
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
