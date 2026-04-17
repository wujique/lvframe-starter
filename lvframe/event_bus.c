#include "event_bus.h"
#include <string.h>
#include <stdio.h>

static EventBus g_bus;

static void event_bus_task(lv_timer_t* timer) {
    (void)timer;
    event_bus_process();
}

void event_bus_init(void) {
    memset(&g_bus, 0, sizeof(EventBus));
    lv_mutex_init(&g_bus.mutex);

    // 创建LVGL定时器，每50ms处理一次事件
    g_bus.process_task = lv_timer_create(event_bus_task, 50, NULL);
    printf("EventBus: initialized, timer=%p\n", g_bus.process_task);
}

void event_bus_subscribe(Page* page, EventType event) {
    if (event < 0 || event >= MAX_EVENT_TYPES) {
        printf("EventBus: invalid event type %d\n", event);
        return;
    }
    if (!page) return;
    
    lv_mutex_lock(&g_bus.mutex);
    
    // 检查是否已经订阅
    for (int i = 0; i < g_bus.sub_count[event]; i++) {
        if (g_bus.subscribers[event][i] == page) {
            printf("EventBus: page=%p already subscribed to event %d\n", page, event);
            lv_mutex_unlock(&g_bus.mutex);
            return;
        }
    }
    
    // 添加到订阅列表
    if (g_bus.sub_count[event] < MAX_SUBSCRIBERS_PER_EVENT) {
        g_bus.subscribers[event][g_bus.sub_count[event]++] = page;
        printf("EventBus: page=%p subscribed to event %d (total subscribers=%d)\n", 
               page, event, g_bus.sub_count[event]);
    } else {
        printf("EventBus: cannot subscribe, max subscribers reached for event %d\n", event);
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}

void event_bus_unsubscribe(Page* page, EventType event) {
    if (event < 0 || event >= MAX_EVENT_TYPES) return;
    if (!page) return;
    
    lv_mutex_lock(&g_bus.mutex);
    
    for (int i = 0; i < g_bus.sub_count[event]; i++) {
        if (g_bus.subscribers[event][i] == page) {
            // 移除订阅者
            memmove(&g_bus.subscribers[event][i], 
                    &g_bus.subscribers[event][i + 1],
                    (g_bus.sub_count[event] - i - 1) * sizeof(Page*));
            g_bus.sub_count[event]--;
            printf("EventBus: page=%p unsubscribed from event %d (remaining=%d)\n", 
                   page, event, g_bus.sub_count[event]);
            break;
        }
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}

void event_bus_unsubscribe_all(Page* page) {
    if (!page) return;
    
    printf("EventBus: unsubscribe all for page=%p\n", page);
    lv_mutex_lock(&g_bus.mutex);
    
    int total_unsub = 0;
    for (int e = 0; e < MAX_EVENT_TYPES; e++) {
        for (int i = 0; i < g_bus.sub_count[e]; i++) {
            if (g_bus.subscribers[e][i] == page) {
                memmove(&g_bus.subscribers[e][i], 
                        &g_bus.subscribers[e][i + 1],
                        (g_bus.sub_count[e] - i - 1) * sizeof(Page*));
                g_bus.sub_count[e]--;
                i--;
                total_unsub++;
            }
        }
    }
    
    lv_mutex_unlock(&g_bus.mutex);
    printf("EventBus: unsubscribed %d events for page=%p\n", total_unsub, page);
}

void event_bus_trigger(Event* event) {
    if (!event) return;
    
    printf("EventBus: trigger event type=%d", event->type);
    switch (event->type) {
        case EVENT_LIGHT_STATE_CHANGED:
            printf(" (LIGHT_STATE_CHANGED) device_id=%d is_on=%d brightness=%d",
                   event->data.light.device_id, event->data.light.is_on, event->data.light.brightness);
            break;
        case EVENT_DEVICE_ADDED:
            printf(" (DEVICE_ADDED) device_id=%d", event->data.device.device_id);
            break;
        case EVENT_DEVICE_REMOVED:
            printf(" (DEVICE_REMOVED) device_id=%d", event->data.device.device_id);
            break;
        case EVENT_NETWORK_DISCONNECTED:
            printf(" (NETWORK_DISCONNECTED)");
            break;
        case EVENT_BATTERY_LOW:
            printf(" (BATTERY_LOW) level=%d", event->data.battery.level);
            break;
        default:
            printf(" (unknown)");
    }
    printf("\n");
    
    lv_mutex_lock(&g_bus.mutex);
    
    if (g_bus.queue_count < MAX_EVENT_QUEUE) {
        g_bus.queue[g_bus.queue_tail] = *event;
        g_bus.queue_tail = (g_bus.queue_tail + 1) % MAX_EVENT_QUEUE;
        g_bus.queue_count++;
        printf("EventBus: event queued, queue_count=%d\n", g_bus.queue_count);
    } else {
        printf("EventBus: queue full, event dropped\n");
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}

void event_bus_process(void) {
    /* 批量处理：每次 timer tick 出队并分发队列中所有积压事件，
     * 避免单事件处理模式下的延迟累积。 */
    while (1) {
        Event event;
        int has_event = 0;

        lv_mutex_lock(&g_bus.mutex);

        if (g_bus.queue_count > 0) {
            event = g_bus.queue[g_bus.queue_head];
            g_bus.queue_head = (g_bus.queue_head + 1) % MAX_EVENT_QUEUE;
            g_bus.queue_count--;
            has_event = 1;
            printf("EventBus: processing event type=%d, queue_count=%d\n", event.type, g_bus.queue_count);
        }

        lv_mutex_unlock(&g_bus.mutex);

        if (!has_event) break;

        if (event.type < 0 || event.type >= MAX_EVENT_TYPES) {
            printf("EventBus: invalid event type %d in queue\n", event.type);
            continue;
        }

        lv_mutex_lock(&g_bus.mutex);
        int subscriber_count = g_bus.sub_count[event.type];
        Page* subscribers_snapshot[MAX_SUBSCRIBERS_PER_EVENT];
        memcpy(subscribers_snapshot, g_bus.subscribers[event.type], subscriber_count * sizeof(Page*));
        lv_mutex_unlock(&g_bus.mutex);

        printf("EventBus: event type=%d has %d subscribers\n", event.type, subscriber_count);

        for (int i = 0; i < subscriber_count; i++) {
            Page* page = subscribers_snapshot[i];
            if (page && page->lifecycle.on_event) {
                printf("EventBus: delivering to page=%p\n", page);
                page->lifecycle.on_event(page, &event);
            }
        }
    }
}

void event_bus_deinit(void) {
    printf("EventBus: deinit\n");
    // 删除定时器
    if (g_bus.process_task) {
        printf("EventBus: deleting timer %p\n", g_bus.process_task);
        lv_timer_delete(g_bus.process_task);
        g_bus.process_task = NULL;
    }
    
    // 删除互斥锁
    lv_mutex_delete(&g_bus.mutex);
    
    // 重置状态（可选）
    memset(&g_bus, 0, sizeof(EventBus));
}