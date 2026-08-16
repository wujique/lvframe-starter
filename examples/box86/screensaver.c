/**
 * @file         screensaver.c
 * @brief        屏保状态机实现，状态转换：NORMAL → SCREENSAVER → BLANK，
 *               由 LVGL 定时器驱动，触摸或外部 wake 命令可唤醒
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */
#include "screensaver.h"
#include "models/model_store.h"
#include "lvframe/page_manager.h"
#include "pages/home_page.h"
#include "lvgl/lvgl.h"

/**
 * @brief        屏保状态枚举
 */
typedef enum {
    SA_STATE_NORMAL      = 0, /**< 正常工作状态 */
    SA_STATE_SCREENSAVER,     /**< 屏保播放中 */
    SA_STATE_BLANK,           /**< 息屏（纯黑）状态 */
} ScreensaverState;

static lv_timer_t*        g_timer      = NULL;            /**< 1s 驱动定时器 */
static model_store_t*     g_store      = NULL;            /**< 模型仓库（读取系统配置） */
static ScreensaverState   g_state      = SA_STATE_NORMAL; /**< 当前状态 */
static int                g_saver_ticks = 0;              /**< 屏保已运行的秒数 */

/**
 * @brief        从 NORMAL 状态进入屏保或直接息屏（取决于系统配置）
 *
 * @return       void
 */
static void enter_screensaver(void)
{
    if (g_state != SA_STATE_NORMAL) return;

    box86_system_model_t sys;
    box86_store_snapshot_system(g_store, &sys);

    g_saver_ticks = 0;
    g_state = SA_STATE_SCREENSAVER;

    if (sys.screensaver_enabled) {
        page_manager_open("Screensaver", NULL);
    } else {
        /* 直接息屏 */
        g_state = SA_STATE_BLANK;
        page_manager_open("BlankScreen", g_store);
    }
}

/**
 * @brief        从 SCREENSAVER 状态进入息屏状态
 *
 * @return       void
 */
static void enter_blank(void)
{
    if (g_state != SA_STATE_SCREENSAVER) return;
    g_state = SA_STATE_BLANK;
    page_manager_open("BlankScreen", g_store);
}

/**
 * @brief        1s 定时器回调，根据当前状态推进屏保状态机
 *
 * @param        timer                LVGL 定时器对象（未使用）
 * @return       void
 */
static void saver_timer_cb(lv_timer_t* timer)
{
    (void)timer;

    box86_system_model_t sys;
    box86_store_snapshot_system(g_store, &sys);

    uint32_t inactive_ms = lv_display_get_inactive_time(NULL);

    switch (g_state) {
    case SA_STATE_NORMAL:
        if (inactive_ms >= (uint32_t)sys.screensaver_timeout * 1000) {
            enter_screensaver();
        }
        break;
    case SA_STATE_SCREENSAVER:
        g_saver_ticks++;
        if (g_saver_ticks >= sys.screensaver_duration) {
            enter_blank();
        }
        break;
    case SA_STATE_BLANK:
        /* 等待用户触摸唤醒或外部 wake 命令 */
        break;
    }
}

/**
 * @brief        初始化屏保状态机，绑定设备仓库并创建 1s LVGL 定时器
 *
 * @param        store                设备数据仓库指针
 * @return       void
 */
void screensaver_init(model_store_t* store)
{
    g_store = store;
    g_state = SA_STATE_NORMAL;
    g_saver_ticks = 0;

    /* 1s 定时器 */
    g_timer = lv_timer_create(saver_timer_cb, 1000, NULL);
}

/**
 * @brief        唤醒屏保，根据 wake_action 配置决定回到首页或重进屏保
 *
 * @return       void
 */
void screensaver_wake(void)
{
    if (g_state == SA_STATE_NORMAL) return;

    box86_system_model_t sys;
    box86_store_snapshot_system(g_store, &sys);

    /* 统一处理页面导航 */
    if (g_state == SA_STATE_BLANK) {
        page_manager_back();   /* 息屏 → 屏保页 (或首页，如果屏保关闭) */
        if (sys.wake_action == 1) {
            /* 回到首页并重置为第一设备页 + 关闭设置面板 */
            page_manager_back_to_home();
            Page* home = page_manager_get_current();
            home_page_reset_to_first(home);
            g_state = SA_STATE_NORMAL;
        } else {
            /* 回到屏保，屏保重新开始计时 */
            g_state = SA_STATE_SCREENSAVER;
        }
    } else if (g_state == SA_STATE_SCREENSAVER) {
        page_manager_back();   /* 屏保 → 首页 */
        g_state = SA_STATE_NORMAL;
    }

    g_saver_ticks = 0;

    /* 重置 LVGL 空闲计时 */
    lv_display_trigger_activity(NULL);
}
