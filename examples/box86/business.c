/**
 * @file         business.c
 * @brief        业务逻辑线程：消费 g_dev_slot、解析 Shell 命令，经 g_ui_slot 通知 UI
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#define _POSIX_C_SOURCE 200809L
#include "business.h"
#include "msg.h"
#include "models/model_base.h"
#include "models/model_store.h"
#include "screensaver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/select.h>
#include <sys/time.h>
#include "lvgl/src/osal/lv_os.h"

/* ── Shell 输出辅助 ── */
static void shell_log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}

/**
 * @brief        设置设备属性并按其类型发送对应刷新信号
 */
static void biz_set_device_prop(Business* biz, void* model, const char* field, int value)
{
    model_base_t* base = (model_base_t*)model;
    if (!base || !base->valid) return;

    int signal = 0;
    int onoff = (strcmp(field, "onoffsta") == 0);

    if (base->type == MODEL_TYPE_LIGHT) {
        box86_store_set_light_prop(biz->store, model, field, value);
        if (onoff) {
            box86_light_model_t s;
            box86_store_snapshot_light(biz->store, model, &s);
            signal = s.onoffsta ? MSG_DEV_LIGHT_ON : MSG_DEV_LIGHT_OFF;
        }
    } else if (base->type == MODEL_TYPE_CCT) {
        box86_store_set_cct_prop(biz->store, model, field, value);
        if (onoff) {
            box86_cct_light_model_t s;
            box86_store_snapshot_cct(biz->store, model, &s);
            signal = s.onoffsta ? MSG_DEV_LIGHT_ON : MSG_DEV_LIGHT_OFF;
        } else if (strcmp(field, "color_temp") == 0) {
            signal = MSG_DEV_CCT_TEMP;
        }
    } else if (base->type == MODEL_TYPE_CURTAIN) {
        box86_store_set_curtain_prop(biz->store, model, field, value);
        signal = MSG_DEV_CURTAIN_POS;
    }

    if (signal) {
        lv_slot_msg_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.signal = signal;
        ev.object = model;
        lv_slot_send(biz->ui_slot, &ev);
    }
}

/**
 * @brief        处理来自 UI 线程的消息
 */
static void handle_ui_msg(Business* biz, const lv_slot_msg_t* msg)
{
    switch (msg->signal) {
    case MSG_UI_SET_PROP: {
        model_base_t* base = (model_base_t*)msg->object;
        biz_set_device_prop(biz, msg->object, msg->field, msg->arg0);
        if (base) shell_log("[UI] set %d %s %d\n", base->id, msg->field, msg->arg0);
        break;
    }
    case MSG_UI_SET_SYSTEM: {
        box86_store_set_system(biz->store, msg->field, msg->arg0);
        shell_log("[UI] set system %s %d\n", msg->field, msg->arg0);

        lv_slot_msg_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.signal = MSG_DEV_REFRESH_SYS;
        ev.object = model_store_get_system(biz->store);
        lv_slot_send(biz->ui_slot, &ev);
        break;
    }
    case MSG_UI_DEL_ACK: {
        model_store_del(biz->store, msg->object);
        shell_log("[BIZ] model freed\n");
        break;
    }
    default:
        break;
    }
}

/**
 * @brief        g_dev_slot 订阅回调：把消息转给 handle_ui_msg
 */
static void biz_msg_handler(void* ctx, const lv_slot_msg_t* msg)
{
    handle_ui_msg((Business*)ctx, msg);
}

/**
 * @brief        解析并执行 Shell 命令行
 */
static void handle_shell_cmd(Business* biz, char* line)
{
    size_t len = strlen(line);
    if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
    if (len == 0) return;

    char cmd[16] = {0};
    sscanf(line, "%15s", cmd);

    /* list */
    if (strcmp(cmd, "list") == 0) {
        void* models[MODEL_STORE_MAX];
        int count = 0;
        model_store_snapshot_ordered(biz->store, models, &count);
        shell_log("%-4s %-12s %-10s %s\n", "ID", "类型", "名称", "状态");
        for (int i = 0; i < count; i++) {
            model_base_t* b = (model_base_t*)models[i];
            const char* type_str =
                b->type == MODEL_TYPE_LIGHT ? "light" :
                b->type == MODEL_TYPE_CCT   ? "cct_light" : "curtain";
            if (b->type == MODEL_TYPE_CURTAIN) {
                box86_curtain_model_t snap;
                box86_store_snapshot_curtain(biz->store, models[i], &snap);
                shell_log("%-4d %-12s %-10s pos=%d\n", b->id, type_str, b->name, snap.position);
            } else if (b->type == MODEL_TYPE_CCT) {
                box86_cct_light_model_t snap;
                box86_store_snapshot_cct(biz->store, models[i], &snap);
                shell_log("%-4d %-12s %-10s %s cct=%d\n", b->id, type_str, b->name,
                          snap.onoffsta ? "on" : "off", snap.color_temp);
            } else {
                box86_light_model_t snap;
                box86_store_snapshot_light(biz->store, models[i], &snap);
                shell_log("%-4d %-12s %-10s %s\n", b->id, type_str, b->name,
                          snap.onoffsta ? "on" : "off");
            }
        }
        return;
    }

    /* get system / get <id> */
    if (strcmp(cmd, "get") == 0) {
        char target[16] = {0};
        sscanf(line, "%*s %15s", target);
        if (strcmp(target, "system") == 0) {
            box86_system_model_t sys;
            box86_store_snapshot_system(biz->store, &sys);
            shell_log("brightness=%d volume=%d network=%d saver=%d timeout=%d duration=%d wake_action=%d\n",
                      sys.brightness, sys.volume, sys.network_enabled,
                      sys.screensaver_enabled, sys.screensaver_timeout,
                      sys.screensaver_duration, sys.wake_action);
        } else {
            int id = atoi(target);
            void* model = model_store_get_by_id(biz->store, id);
            if (!model) { shell_log("error: device %d not found\n", id); return; }
            model_base_t* b = (model_base_t*)model;
            if (b->type == MODEL_TYPE_CURTAIN) {
                box86_curtain_model_t snap; box86_store_snapshot_curtain(biz->store, model, &snap);
                shell_log("id=%d name=%s pos=%d\n", id, b->name, snap.position);
            } else if (b->type == MODEL_TYPE_CCT) {
                box86_cct_light_model_t snap; box86_store_snapshot_cct(biz->store, model, &snap);
                shell_log("id=%d name=%s onoff=%d cct=%d\n", id, b->name, snap.onoffsta, snap.color_temp);
            } else {
                box86_light_model_t snap; box86_store_snapshot_light(biz->store, model, &snap);
                shell_log("id=%d name=%s onoff=%d\n", id, b->name, snap.onoffsta);
            }
        }
        return;
    }

    /* add <type> <name> */
    if (strcmp(cmd, "add") == 0) {
        char type_str[16] = {0}, name[MODEL_NAME_MAX] = {0};
        sscanf(line, "%*s %15s %31[^\n]", type_str, name);
        int new_id = -1;
        if      (strcmp(type_str, "light")     == 0) new_id = box86_store_add_light(biz->store, name);
        else if (strcmp(type_str, "cct_light") == 0) new_id = box86_store_add_cct(biz->store, name);
        else if (strcmp(type_str, "curtain")   == 0) new_id = box86_store_add_curtain(biz->store, name);
        else { shell_log("error: unknown type '%s'\n", type_str); return; }

        if (new_id < 0) { shell_log("error: max devices reached\n"); return; }
        shell_log("added device id=%d\n", new_id);

        void* model = model_store_get_by_id(biz->store, new_id);
        lv_slot_msg_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.signal = MSG_DEV_ADD_MODEL;
        ev.object = model;
        lv_slot_send(biz->ui_slot, &ev);
        return;
    }

    /* del <id> */
    if (strcmp(cmd, "del") == 0) {
        int id = 0;
        sscanf(line, "%*s %d", &id);
        void* model = model_store_get_by_id(biz->store, id);
        if (!model) { shell_log("error: device %d not found\n", id); return; }
        lv_slot_msg_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.signal = MSG_DEV_DEL_MODEL;
        ev.object = model;
        lv_slot_send(biz->ui_slot, &ev);
        return;
    }

    /* move <id> <pos> */
    if (strcmp(cmd, "move") == 0) {
        int id = 0, pos = 0;
        sscanf(line, "%*s %d %d", &id, &pos);
        void* model = model_store_get_by_id(biz->store, id);
        if (!model) { shell_log("error: device %d not found\n", id); return; }
        model_store_move(biz->store, model, pos);

        lv_slot_msg_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.signal = MSG_DEV_MOVE_MODEL;
        ev.object = model;
        ev.arg0 = pos;
        lv_slot_send(biz->ui_slot, &ev);
        return;
    }

    /* set <id|system> <field> <value> */
    if (strcmp(cmd, "set") == 0) {
        char target[16] = {0}, field[32] = {0}, val_str[32] = {0};
        sscanf(line, "%*s %15s %31s %31s", target, field, val_str);

        if (strcmp(target, "system") == 0) {
            int value = atoi(val_str);
            box86_store_set_system(biz->store, field, value);
            shell_log("system %s=%d\n", field, value);

            lv_slot_msg_t ev;
            memset(&ev, 0, sizeof(ev));
            ev.signal = MSG_DEV_REFRESH_SYS;
            ev.object = model_store_get_system(biz->store);
            lv_slot_send(biz->ui_slot, &ev);
        } else {
            int id = atoi(target);
            void* model = model_store_get_by_id(biz->store, id);
            if (!model) { shell_log("error: device %d not found\n", id); return; }

            int value = 0;
            if (strcmp(field, "command") == 0) {
                if      (strcmp(val_str, "OPEN")  == 0) value = BOX86_CURTAIN_CMD_OPEN;
                else if (strcmp(val_str, "CLOSE") == 0) value = BOX86_CURTAIN_CMD_CLOSE;
                else if (strcmp(val_str, "STOP")  == 0) value = BOX86_CURTAIN_CMD_STOP;
            } else {
                value = atoi(val_str);
            }

            model_base_t* b = (model_base_t*)model;
            if (strcmp(field, "onoffsta") == 0) {
                biz_set_device_prop(biz, model, field, value);
            } else if (strcmp(field, "color_temp") == 0 && b->type == MODEL_TYPE_CCT) {
                biz_set_device_prop(biz, model, field, value);
            } else if ((strcmp(field, "position") == 0 || strcmp(field, "command") == 0) &&
                       b->type == MODEL_TYPE_CURTAIN) {
                biz_set_device_prop(biz, model, field, value);
            } else {
                shell_log("error: invalid field or device\n");
                return;
            }
            shell_log("device %d %s=%d\n", id, field, value);
        }
        return;
    }

    /* wake — 模拟触摸唤醒 */
    if (strcmp(cmd, "wake") == 0) {
        screensaver_wake();
        shell_log("wake triggered\n");
        return;
    }

    shell_log("unknown command: %s\n", cmd);
    shell_log("commands: list | get <id|system> | add <type> <name> | del <id> | move <id> <pos> | set <id|system> <field> <value> | wake\n");
}

/**
 * @brief        业务逻辑线程主循环：交替消费 g_dev_slot 与 Shell 输入
 */
static void business_thread(void* arg)
{
    Business* biz = (Business*)arg;
    char line[256];

    while (biz->running) {
        /* 消费来自 UI 的消息（非阻塞批量处理） */
        lv_slot_process(biz->dev_slot);

        /* 非阻塞读取 Shell 输入 */
        fd_set fds;
        struct timeval tv;
        FD_ZERO(&fds);
        FD_SET(0, &fds); /* stdin */
        tv.tv_sec = 0;
        tv.tv_usec = 10000; /* 10ms 超时 */

        int ret = select(1, &fds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(0, &fds)) {
            if (fgets(line, sizeof(line), stdin)) {
                handle_shell_cmd(biz, line);
            }
        }

        lv_sleep_ms(5);
    }
}

void business_init(Business* biz, lv_slot_t* dev_slot, lv_slot_t* ui_slot, model_store_t* store)
{
    memset(biz, 0, sizeof(*biz));
    biz->dev_slot = dev_slot;
    biz->ui_slot  = ui_slot;
    biz->store    = store;

    /* 业务逻辑订阅 g_dev_slot（object=NULL 全收），由 DEV 线程消费分发 */
    lv_slot_subscribe(dev_slot, MSG_UI_SET_PROP,  NULL, biz_msg_handler, biz);
    lv_slot_subscribe(dev_slot, MSG_UI_SET_SYSTEM, NULL, biz_msg_handler, biz);
    lv_slot_subscribe(dev_slot, MSG_UI_DEL_ACK,   NULL, biz_msg_handler, biz);

    /* stdout 无缓冲，确保 shell 输出立即显示 */
    setvbuf(stdout, NULL, _IONBF, 0);
}

void business_start(Business* biz)
{
    biz->running = 1;
    lv_thread_init(&biz->thread, "business", LV_THREAD_PRIO_MID, business_thread, 4096, biz);
}

void business_stop(Business* biz)
{
    biz->running = 0;
    lv_thread_delete(&biz->thread);
}
