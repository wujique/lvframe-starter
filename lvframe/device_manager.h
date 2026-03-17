#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "event_bus.h"

#define MAX_LIGHTS 10

typedef struct LightModel {
    int id;
    char name[16];
    int is_on;
    int brightness;
    int ref_count;
    int is_valid;
} LightModel;

typedef struct {
    LightModel* lights[MAX_LIGHTS];
    int light_count;
    lv_thread_t thread;
    int running;
    void* cmd_queue;  // 简化的命令队列
} DeviceManager;

DeviceManager* device_manager_create(void);
void device_manager_destroy(DeviceManager* dm);
void device_manager_start(DeviceManager* dm);
void device_manager_stop(DeviceManager* dm);
LightModel* device_manager_get_light(DeviceManager* dm, int id);
void device_manager_send_command(DeviceManager* dm, int device_id, int cmd, int value);

#endif