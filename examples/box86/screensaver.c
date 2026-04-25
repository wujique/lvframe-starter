#include "screensaver.h"
#include "models/device_store.h"
#include "lvframe/page_manager.h"
#include "pages/home_page.h"
#include "lvgl/lvgl.h"

typedef enum {
    SA_STATE_NORMAL = 0,
    SA_STATE_SCREENSAVER,
    SA_STATE_BLANK,
} ScreensaverState;

static lv_timer_t*        g_timer = NULL;
static lv_device_store_t* g_store = NULL;
static ScreensaverState   g_state = SA_STATE_NORMAL;
static int                g_saver_ticks = 0;

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

static void enter_blank(void)
{
    if (g_state != SA_STATE_SCREENSAVER) return;
    g_state = SA_STATE_BLANK;
    page_manager_open("BlankScreen", g_store);
}

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

void screensaver_init(lv_device_store_t* store)
{
    g_store = store;
    g_state = SA_STATE_NORMAL;
    g_saver_ticks = 0;

    /* 1s 定时器 */
    g_timer = lv_timer_create(saver_timer_cb, 1000, NULL);
}

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
