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

/**
 * EventType — 框架预留事件类型
 *
 * 框架本身不定义任何业务事件。应用层从 EVENT_USER_START 开始定义自己的事件类型。
 * 例如：
 *   typedef enum {
 *       MY_EVENT_FOO = EVENT_USER_START,
 *       MY_EVENT_BAR,
 *   } MyEventType;
 */
typedef enum {
    EVENT_USER_START = 0   /* 应用层自定义事件从此开始 */
} EventType;

typedef struct {
    int type;    /* 实际为 EventType 或应用层扩展值 */
    union {
        /* 通用用户数据载体，应用层可自由定义语义 */
        struct { int msg_type; int device_id; char field[32]; int value; int new_pos; } user;
        /* 预留扩展槽，避免应用层直接操作原始字节 */
        int  i[8];
        void* p[4];
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

/**
 * 注册 root 对象创建后的回调，可用于应用层设置全局字体等。
 * 每次 page_create 创建 root 后都会调用此回调。
 * @param cb  回调函数，参数为新创建的 root 对象；传 NULL 清除回调
 */
void page_set_root_created_cb(void (*cb)(lv_obj_t *root));

#endif /* PAGE_H */
