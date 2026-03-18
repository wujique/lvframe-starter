#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "page.h"

/* EventType 和 Event 均定义在 page.h 中，此处直接使用 */

#define MAX_EVENT_TYPES          EVENT_COUNT
#define MAX_SUBSCRIBERS_PER_EVENT 10
#define MAX_EVENT_QUEUE          50

typedef struct {
    Page* subscribers[MAX_EVENT_TYPES][MAX_SUBSCRIBERS_PER_EVENT];
    int   sub_count[MAX_EVENT_TYPES];
    Event queue[MAX_EVENT_QUEUE];
    int   queue_head;
    int   queue_tail;
    int   queue_count;
    lv_mutex_t  mutex;
    lv_timer_t* process_task;
} EventBus;

void event_bus_init(void);
void event_bus_subscribe(Page* page, EventType event_type);
void event_bus_unsubscribe(Page* page, EventType event_type);
void event_bus_unsubscribe_all(Page* page);
void event_bus_trigger(Event* event);
void event_bus_process(void);

#endif /* EVENT_BUS_H */
