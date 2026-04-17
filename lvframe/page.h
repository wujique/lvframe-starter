#ifndef PAGE_H
#define PAGE_H

#include "../lvgl/lvgl/lvgl.h"
#include "../lvgl/lvgl/src/osal/lv_os_private.h"

typedef enum {
    PAGE_STATE_NONE = 0,
    PAGE_STATE_CREATED,
    PAGE_STATE_STARTED,
    PAGE_STATE_RESUMED,
    PAGE_STATE_PAUSED,
    PAGE_STATE_STOPPED,
    PAGE_STATE_DESTROYED
} PageState;

typedef struct Page Page;

/* ── 事件系统 ── */
typedef enum {
    EVENT_LIGHT_STATE_CHANGED,
    EVENT_DEVICE_ADDED,
    EVENT_DEVICE_REMOVED,
    EVENT_NETWORK_DISCONNECTED,
    EVENT_BATTERY_LOW,
    EVENT_COUNT,
    EVENT_USER_START = 100     /* 应用层自定义事件从此开始 */
} EventType;

typedef struct {
    EventType type;
    union {
        struct { int device_id; int is_on; int brightness; } light;
        struct { int device_id; } device;
        struct { int level; } battery;
        struct { int msg_type; int device_id; char field[32]; int value; int new_pos; } user;
    } data;
} Event;

typedef struct {
    void (*on_create)(Page* page, void* params);
    void (*on_start)(Page* page);
    void (*on_resume)(Page* page);
    void (*on_pause)(Page* page);
    void (*on_stop)(Page* page);
    void (*on_destroy)(Page* page);
    void (*on_event)(Page* page, Event* event);
} PageLifecycle;

struct Page {
    lv_obj_t*     root;
    PageState     state;
    void*         user_data;
    PageLifecycle lifecycle;
    int           model_valid;  /* 0:无效 1:有效 */
};

Page* page_create(PageLifecycle* lifecycle, void* params);
void  page_destroy(Page* page);
lv_obj_t* page_get_root(Page* page);
void  page_set_user_data(Page* page, void* data);
void* page_get_user_data(Page* page);
void  page_set_model_valid(Page* page, int valid);
int   page_is_model_valid(Page* page);

#endif /* PAGE_H */
