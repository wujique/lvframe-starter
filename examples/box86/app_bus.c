#include "app_bus.h"
#include <string.h>

static void queue_init(AppQueue* q)
{
    memset(q, 0, sizeof(AppQueue));
    lv_mutex_init(&q->mutex);
    lv_thread_sync_init(&q->sem);
}

static void queue_deinit(AppQueue* q)
{
    lv_mutex_delete(&q->mutex);
    lv_thread_sync_delete(&q->sem);
}

static int queue_push(AppQueue* q, const AppMsg* msg)
{
    lv_mutex_lock(&q->mutex);
    if (q->count >= APP_BUS_QUEUE_SIZE) {
        lv_mutex_unlock(&q->mutex);
        return -1;
    }
    q->buf[q->tail] = *msg;
    q->tail = (q->tail + 1) % APP_BUS_QUEUE_SIZE;
    q->count++;
    lv_mutex_unlock(&q->mutex);
    lv_thread_sync_signal(&q->sem);
    return 0;
}

static int queue_pop(AppQueue* q, AppMsg* out)
{
    lv_mutex_lock(&q->mutex);
    if (q->count == 0) {
        lv_mutex_unlock(&q->mutex);
        return 0;
    }
    *out = q->buf[q->head];
    q->head = (q->head + 1) % APP_BUS_QUEUE_SIZE;
    q->count--;
    lv_mutex_unlock(&q->mutex);
    return 1;
}

void app_bus_init(AppBus* bus)
{
    queue_init(&bus->ui_to_biz);
    queue_init(&bus->biz_to_ui);
}

void app_bus_deinit(AppBus* bus)
{
    queue_deinit(&bus->ui_to_biz);
    queue_deinit(&bus->biz_to_ui);
}

int app_bus_send_ui(AppBus* bus, const AppMsg* msg)
{
    return queue_push(&bus->ui_to_biz, msg);
}

int app_bus_send_biz(AppBus* bus, const AppMsg* msg)
{
    return queue_push(&bus->biz_to_ui, msg);
}

int app_bus_recv_ui(AppBus* bus, AppMsg* out)
{
    return queue_pop(&bus->ui_to_biz, out);
}

int app_bus_recv_biz(AppBus* bus, AppMsg* out)
{
    return queue_pop(&bus->biz_to_ui, out);
}
