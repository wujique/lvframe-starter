#include "app_bus_adapter.h"
#include <string.h>
#include <stdio.h>

AppBusAdapter g_app_bus_adapter;

static void app_bus_adapter_timer_cb(lv_timer_t* timer) {
    AppBusAdapter* adapter = lv_timer_get_user_data(timer);
    if (adapter) {
        app_bus_adapter_poll(adapter);
    }
}

void app_bus_adapter_init(AppBusAdapter* adapter, AppBus* bus) {
    if (!adapter || !bus) return;
    memset(adapter, 0, sizeof(AppBusAdapter));
    adapter->app_bus = bus;
}

void app_bus_adapter_deinit(AppBusAdapter* adapter) {
    if (!adapter) return;
    app_bus_adapter_stop_auto_poll(adapter);
    memset(adapter, 0, sizeof(AppBusAdapter));
}

void app_bus_adapter_poll(AppBusAdapter* adapter) {
    if (!adapter || !adapter->app_bus) return;

    AppMsg msg;
    while (app_bus_recv_biz(adapter->app_bus, &msg)) {
        Event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = EVENT_APP_MESSAGE;
        ev.data.user.msg_type  = msg.type;
        ev.data.user.device_id = msg.device_id;
        ev.data.user.value     = msg.value;
        ev.data.user.new_pos   = msg.new_pos;
        strncpy(ev.data.user.field, msg.field, sizeof(ev.data.user.field) - 1);
        ev.data.user.field[sizeof(ev.data.user.field) - 1] = '\0';

        event_bus_trigger(&ev);
        event_bus_process();
    }
}

void app_bus_adapter_start_auto_poll(AppBusAdapter* adapter, uint32_t interval_ms) {
    if (!adapter || adapter->poll_timer) return;
    adapter->poll_timer = lv_timer_create(app_bus_adapter_timer_cb, interval_ms, adapter);
    /* -1 表示无限重复；0 会导致 timer 立即被 LVGL 删除而永不执行 */
    lv_timer_set_repeat_count(adapter->poll_timer, -1);
}

void app_bus_adapter_stop_auto_poll(AppBusAdapter* adapter) {
    if (!adapter || !adapter->poll_timer) return;
    lv_timer_delete(adapter->poll_timer);
    adapter->poll_timer = NULL;
}
