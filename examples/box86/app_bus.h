#ifndef APP_BUS_H
#define APP_BUS_H

/**
 * app_bus — 应用层跨线程消息总线
 *
 * 两个方向各一个队列：
 *   UI  → 业务：ui_to_biz_queue
 *   业务 → UI ：biz_to_ui_queue
 *
 * 所有接口线程安全。
 */

#include "models/device_model.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/osal/lv_os_private.h"

/* ── 消息类型 ── */
typedef enum {
    /* UI → 业务 */
    MSG_UI_SET_PROP,      /* 设备属性变更 */
    MSG_UI_SET_SYSTEM,    /* 系统属性变更 */

    /* 业务 → UI */
    MSG_BIZ_REFRESH,      /* 刷新指定设备页 */
    MSG_BIZ_ADD_DEVICE,   /* 新增设备页 */
    MSG_BIZ_DEL_DEVICE,   /* 删除设备页（UI 销毁后需回复 MSG_UI_DEL_ACK） */
    MSG_BIZ_MOVE_DEVICE,  /* 调整设备页顺序 */
    MSG_BIZ_REFRESH_SYS,  /* 刷新系统状态（设置页） */

    /* UI → 业务（删除确认） */
    MSG_UI_DEL_ACK,       /* UI 已销毁设备页，业务可释放模型 */
} AppMsgType;

typedef struct {
    AppMsgType type;
    int        device_id;   /* 目标设备 id（-1 表示系统） */
    char       field[32];   /* 属性名 */
    int        value;       /* 属性值 */
    int        new_pos;     /* move 时的目标位置 */
} AppMsg;

#define APP_BUS_QUEUE_SIZE 64

typedef struct {
    AppMsg     buf[APP_BUS_QUEUE_SIZE];
    int        head, tail, count;
    lv_mutex_t mutex;
    lv_thread_sync_t sem;  /* 业务线程等待 Shell 输入或消息 */
} AppQueue;

typedef struct {
    AppQueue ui_to_biz;
    AppQueue biz_to_ui;
} AppBus;

void app_bus_init(AppBus* bus);
void app_bus_deinit(AppBus* bus);

/* 发送（线程安全） */
int  app_bus_send_ui(AppBus* bus, const AppMsg* msg);   /* UI  → 业务 */
int  app_bus_send_biz(AppBus* bus, const AppMsg* msg);  /* 业务 → UI  */

/* 接收（线程安全，无消息返回 0） */
int  app_bus_recv_ui(AppBus* bus, AppMsg* out);   /* 业务线程调用 */
int  app_bus_recv_biz(AppBus* bus, AppMsg* out);  /* LVGL 线程调用 */

#endif /* APP_BUS_H */
