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
}

void event_bus_subscribe(Page* page, EventType event) {
    if (event < 0 || event >= EVENT_COUNT) return;
    if (!page) return;
    
    lv_mutex_lock(&g_bus.mutex);
    
    // 检查是否已经订阅
    for (int i = 0; i < g_bus.sub_count[event]; i++) {
        if (g_bus.subscribers[event][i] == page) {
            lv_mutex_unlock(&g_bus.mutex);
            return;
        }
    }
    
    // 添加到订阅列表
    if (g_bus.sub_count[event] < MAX_SUBSCRIBERS_PER_EVENT) {
        g_bus.subscribers[event][g_bus.sub_count[event]++] = page;
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}

void event_bus_unsubscribe(Page* page, EventType event) {
    if (event < 0 || event >= EVENT_COUNT) return;
    if (!page) return;
    
    lv_mutex_lock(&g_bus.mutex);
    
    for (int i = 0; i < g_bus.sub_count[event]; i++) {
        if (g_bus.subscribers[event][i] == page) {
            // 移除订阅者
            memmove(&g_bus.subscribers[event][i], 
                    &g_bus.subscribers[event][i + 1],
                    (g_bus.sub_count[event] - i - 1) * sizeof(Page*));
            g_bus.sub_count[event]--;
            break;
        }
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}

void event_bus_unsubscribe_all(Page* page) {
    if (!page) return;
    
    lv_mutex_lock(&g_bus.mutex);
    
    for (int e = 0; e < EVENT_COUNT; e++) {
        for (int i = 0; i < g_bus.sub_count[e]; i++) {
            if (g_bus.subscribers[e][i] == page) {
                memmove(&g_bus.subscribers[e][i], 
                        &g_bus.subscribers[e][i + 1],
                        (g_bus.sub_count[e] - i - 1) * sizeof(Page*));
                g_bus.sub_count[e]--;
                i--;
            }
        }
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}

void event_bus_trigger(Event* event) {
    if (!event) return;
    
    lv_mutex_lock(&g_bus.mutex);
    
    if (g_bus.queue_count < MAX_EVENT_QUEUE) {
        g_bus.queue[g_bus.queue_tail] = *event;
        g_bus.queue_tail = (g_bus.queue_tail + 1) % MAX_EVENT_QUEUE;
        g_bus.queue_count++;
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}

void event_bus_process(void) {
    Event event;
    int has_event = 0;
    
    lv_mutex_lock(&g_bus.mutex);
    
    if (g_bus.queue_count > 0) {
        event = g_bus.queue[g_bus.queue_head];
        g_bus.queue_head = (g_bus.queue_head + 1) % MAX_EVENT_QUEUE;
        g_bus.queue_count--;
        has_event = 1;
    }
    
    lv_mutex_unlock(&g_bus.mutex);
    
    if (!has_event) return;
    
    // 分发事件给订阅者
    lv_mutex_lock(&g_bus.mutex);
    
    for (int i = 0; i < g_bus.sub_count[event.type]; i++) {
        Page* page = g_bus.subscribers[event.type][i];
        if (page && page->lifecycle.on_event) {
            page->lifecycle.on_event(page, &event);
        }
    }
    
    lv_mutex_unlock(&g_bus.mutex);
}