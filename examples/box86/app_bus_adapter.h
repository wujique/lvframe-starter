#ifndef APP_BUS_ADAPTER_H
#define APP_BUS_ADAPTER_H

#include "app_bus.h"
#include "lvframe/event_bus.h"

/**
 * app_bus_adapter - 将 AppBus biz_to_ui 消息转换为 EventBus 事件
 *
 * 轮询 AppBus 的 biz_to_ui 队列，将每条 AppMsg 转换为
 * EVENT_APP_MESSAGE 事件并通过 EventBus 分发给订阅页面。
 *
 * 页面通过 event_bus_subscribe(page, EVENT_APP_MESSAGE) 订阅，
 * 在 on_event 回调中按 event->data.user.msg_type 过滤处理。
 */

typedef struct {
    AppBus*      app_bus;
    lv_timer_t*  poll_timer;
} AppBusAdapter;

extern AppBusAdapter g_app_bus_adapter;

void app_bus_adapter_init(AppBusAdapter* adapter, AppBus* bus);
void app_bus_adapter_deinit(AppBusAdapter* adapter);
void app_bus_adapter_poll(AppBusAdapter* adapter);
void app_bus_adapter_start_auto_poll(AppBusAdapter* adapter, uint32_t interval_ms);
void app_bus_adapter_stop_auto_poll(AppBusAdapter* adapter);

#endif /* APP_BUS_ADAPTER_H */
