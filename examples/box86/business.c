#define _POSIX_C_SOURCE 200809L
#include "business.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* ── Shell 输出辅助 ── */
static void shell_log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}

/* ── 处理来自 UI 的消息 ── */
static void handle_ui_msg(Business* biz, const AppMsg* msg)
{
    switch (msg->type) {
    case MSG_UI_SET_PROP: {
        /* 按设备类型分发 */
        DeviceBase bases[MAX_DEVICES];
        int order[MAX_DEVICES], count = 0;
        device_store_snapshot_base(biz->store, bases, order, &count);
        DeviceType dtype = DEVICE_TYPE_LIGHT;
        for (int i = 0; i < count; i++) if (bases[i].id == msg->device_id) { dtype = bases[i].type; break; }

        if      (dtype == DEVICE_TYPE_LIGHT)   device_store_set_light_prop(biz->store, msg->device_id, msg->field, msg->value);
        else if (dtype == DEVICE_TYPE_CCT)     device_store_set_cct_prop(biz->store, msg->device_id, msg->field, msg->value);
        else if (dtype == DEVICE_TYPE_CURTAIN) device_store_set_curtain_prop(biz->store, msg->device_id, msg->field, msg->value);

        shell_log("[UI] set %d %s %d\n", msg->device_id, msg->field, msg->value);
        break;
    }
    case MSG_UI_SET_SYSTEM:
        device_store_set_system(biz->store, msg->field, msg->value);
        shell_log("[UI] set system %s %d\n", msg->field, msg->value);
        break;
    case MSG_UI_DEL_ACK:
        /* UI 已销毁页面，现在可以安全删除模型 */
        device_store_del(biz->store, msg->device_id);
        shell_log("[BIZ] device %d model freed\n", msg->device_id);
        break;
    default:
        break;
    }
}

/* ── Shell 命令解析 ── */
static void handle_shell_cmd(Business* biz, char* line)
{
    /* 去掉末尾换行 */
    size_t len = strlen(line);
    if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
    if (len == 0) return;

    char cmd[16] = {0};
    sscanf(line, "%15s", cmd);

    AppMsg msg = {0};

    /* list */
    if (strcmp(cmd, "list") == 0) {
        DeviceBase bases[MAX_DEVICES];
        int order[MAX_DEVICES], count = 0;
        device_store_snapshot_base(biz->store, bases, order, &count);
        shell_log("%-4s %-12s %-10s %s\n", "ID", "类型", "名称", "状态");
        for (int i = 0; i < count; i++) {
            DeviceBase* b = &bases[order[i]];
            const char* type_str =
                b->type == DEVICE_TYPE_LIGHT   ? "light" :
                b->type == DEVICE_TYPE_CCT     ? "cct_light" : "curtain";
            if (b->type == DEVICE_TYPE_CURTAIN) {
                CurtainModel snap;
                device_store_snapshot_curtain(biz->store, b->id, &snap);
                shell_log("%-4d %-12s %-10s pos=%d\n", b->id, type_str, b->name, snap.position);
            } else if (b->type == DEVICE_TYPE_CCT) {
                CctLightModel snap;
                device_store_snapshot_cct(biz->store, b->id, &snap);
                shell_log("%-4d %-12s %-10s %s cct=%d\n", b->id, type_str, b->name,
                          snap.onoffsta ? "on" : "off", snap.color_temp);
            } else {
                LightModel snap;
                device_store_snapshot_light(biz->store, b->id, &snap);
                shell_log("%-4d %-12s %-10s %s\n", b->id, type_str, b->name,
                          snap.onoffsta ? "on" : "off");
            }
        }
        return;
    }

    /* get system */
    if (strcmp(cmd, "get") == 0) {
        char target[16] = {0};
        sscanf(line, "%*s %15s", target);
        if (strcmp(target, "system") == 0) {
            SystemModel sys;
            device_store_snapshot_system(biz->store, &sys);
            shell_log("brightness=%d volume=%d network=%d\n",
                      sys.brightness, sys.volume, sys.network_enabled);
        } else {
            int id = atoi(target);
            DeviceBase bases[MAX_DEVICES];
            int order[MAX_DEVICES], count = 0;
            device_store_snapshot_base(biz->store, bases, order, &count);
            DeviceBase* b = NULL;
            for (int i = 0; i < count; i++) if (bases[i].id == id) { b = &bases[i]; break; }
            if (!b) { shell_log("error: device %d not found\n", id); return; }
            if (b->type == DEVICE_TYPE_CURTAIN) {
                CurtainModel snap; device_store_snapshot_curtain(biz->store, id, &snap);
                shell_log("id=%d name=%s pos=%d\n", id, b->name, snap.position);
            } else if (b->type == DEVICE_TYPE_CCT) {
                CctLightModel snap; device_store_snapshot_cct(biz->store, id, &snap);
                shell_log("id=%d name=%s onoff=%d cct=%d\n", id, b->name, snap.onoffsta, snap.color_temp);
            } else {
                LightModel snap; device_store_snapshot_light(biz->store, id, &snap);
                shell_log("id=%d name=%s onoff=%d\n", id, b->name, snap.onoffsta);
            }
        }
        return;
    }

    /* add <type> <name> */
    if (strcmp(cmd, "add") == 0) {
        char type_str[16] = {0}, name[DEVICE_NAME_MAX] = {0};
        sscanf(line, "%*s %15s %31[^\n]", type_str, name);
        int new_id = -1;
        if      (strcmp(type_str, "light")     == 0) new_id = device_store_add_light(biz->store, name);
        else if (strcmp(type_str, "cct_light") == 0) new_id = device_store_add_cct(biz->store, name);
        else if (strcmp(type_str, "curtain")   == 0) new_id = device_store_add_curtain(biz->store, name);
        else { shell_log("error: unknown type '%s'\n", type_str); return; }

        if (new_id < 0) { shell_log("error: max devices reached\n"); return; }
        shell_log("added device id=%d\n", new_id);

        msg.type      = MSG_BIZ_ADD_DEVICE;
        msg.device_id = new_id;
        app_bus_send_biz(biz->bus, &msg);
        return;
    }

    /* del <id> */
    if (strcmp(cmd, "del") == 0) {
        int id = 0;
        sscanf(line, "%*s %d", &id);
        /* 先通知 UI 销毁页面；模型在收到 MSG_UI_DEL_ACK 后再释放 */
        msg.type      = MSG_BIZ_DEL_DEVICE;
        msg.device_id = id;
        app_bus_send_biz(biz->bus, &msg);
        return;
    }

    /* move <id> <pos> */
    if (strcmp(cmd, "move") == 0) {
        int id = 0, pos = 0;
        sscanf(line, "%*s %d %d", &id, &pos);
        device_store_move(biz->store, id, pos);
        msg.type      = MSG_BIZ_MOVE_DEVICE;
        msg.device_id = id;
        msg.new_pos   = pos;
        app_bus_send_biz(biz->bus, &msg);
        return;
    }

    /* set <id|system> <field> <value> */
    if (strcmp(cmd, "set") == 0) {
        char target[16] = {0}, field[32] = {0}, val_str[32] = {0};
        sscanf(line, "%*s %15s %31s %31s", target, field, val_str);

        if (strcmp(target, "system") == 0) {
            int value = atoi(val_str);
            device_store_set_system(biz->store, field, value);
            shell_log("system %s=%d\n", field, value);
            msg.type = MSG_BIZ_REFRESH_SYS;
            app_bus_send_biz(biz->bus, &msg);
        } else {
            int id = atoi(target);
            int value = 0;
            /* command 字段特殊处理 */
            if (strcmp(field, "command") == 0) {
                if      (strcmp(val_str, "OPEN")  == 0) value = CURTAIN_CMD_OPEN;
                else if (strcmp(val_str, "CLOSE") == 0) value = CURTAIN_CMD_CLOSE;
                else if (strcmp(val_str, "STOP")  == 0) value = CURTAIN_CMD_STOP;
            } else {
                value = atoi(val_str);
            }
            /* 按设备类型分发 set_prop */
            DeviceBase bases[MAX_DEVICES];
            int order2[MAX_DEVICES], count2 = 0;
            device_store_snapshot_base(biz->store, bases, order2, &count2);
            DeviceType dtype = DEVICE_TYPE_LIGHT;
            for (int i = 0; i < count2; i++) if (bases[i].id == id) { dtype = bases[i].type; break; }

            int ret = -1;
            if      (dtype == DEVICE_TYPE_LIGHT)   ret = device_store_set_light_prop(biz->store, id, field, value);
            else if (dtype == DEVICE_TYPE_CCT)     ret = device_store_set_cct_prop(biz->store, id, field, value);
            else if (dtype == DEVICE_TYPE_CURTAIN) ret = device_store_set_curtain_prop(biz->store, id, field, value);

            if (ret < 0) { shell_log("error: invalid field or device\n"); return; }
            shell_log("device %d %s=%d\n", id, field, value);
            msg.type      = MSG_BIZ_REFRESH;
            msg.device_id = id;
            app_bus_send_biz(biz->bus, &msg);
        }
        return;
    }

    shell_log("unknown command: %s\n", cmd);
    shell_log("commands: list | get <id|system> | add <type> <name> | del <id> | move <id> <pos> | set <id|system> <field> <value>\n");
}

/* ── 业务线程主循环 ── */
static void business_thread(void* arg)
{
    Business* biz = (Business*)arg;
    char line[256];

    while (biz->running) {
        /* 处理来自 UI 的消息（非阻塞） */
        AppMsg msg;
        while (app_bus_recv_ui(biz->bus, &msg)) {
            handle_ui_msg(biz, &msg);
        }

        /* 读取 Shell 输入（非阻塞，使用 select 或直接 fgets） */
        /* 简化实现：fgets 会阻塞，生产环境可用 select/poll */
        if (fgets(line, sizeof(line), stdin)) {
            handle_shell_cmd(biz, line);
        }
    }
}

void business_init(Business* biz, AppBus* bus, DeviceStore* store)
{
    memset(biz, 0, sizeof(Business));
    biz->bus   = bus;
    biz->store = store;
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
