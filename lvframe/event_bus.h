#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "page.h"

#define MAX_EVENT_TYPES 20
#define MAX_SUBSCRIBERS_PER_EVENT 10
#define MAX_EVENT_QUEUE 50

typedef enum {
    EVENT_LIGHT_STATE_CHANGED,
    EVENT_DEVICE_ADDED,
    EVENT_DEVICE_REMOVED,
    EVENT_NETWORK_DISCONNECTED,
    EVENT_BATTERY_LOW,
    EVENT_COUNT
} EventType;

typedef struct {
    EventType type;
    union {
        struct { int device_id; int is_on; int brightness; } light;
        struct { int device_id; } device;
        struct { int level; } battery;
    } data;
} Event;

typedef struct {
    Page* subscribers[MAX_EVENT_TYPES][MAX_SUBSCRIBERS_PER_EVENT];
    int sub_count[MAX_EVENT_TYPES];
    Event queue[MAX_EVENT_QUEUE];
    int queue_head;
    int queue_tail;
    int queue_count;
    lv_mutex_t mutex;
    lv_timer_t* process_task;
} EventBus;

void event_bus_init(void);
void event_bus_subscribe(Page* page, EventType event);
void event_bus_unsubscribe(Page* page, EventType event);
void event_bus_unsubscribe_all(Page* page);
void event_bus_trigger(Event* event);
void event_bus_process(void);

#endif