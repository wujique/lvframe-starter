#include "device_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 简化的命令队列
typedef struct {
    int device_id;
    int cmd;
    int value;
} Command;

#define CMD_QUEUE_SIZE 20

static Command g_cmd_queue[CMD_QUEUE_SIZE];
static int g_cmd_head = 0;
static int g_cmd_tail = 0;
static int g_cmd_count = 0;
static lv_mutex_t g_cmd_mutex;

static void device_thread_entry(void* arg) {
    DeviceManager* dm = (DeviceManager*)arg;
    printf("DeviceManager: device thread started dm=%p\n", dm);
    
    while (dm->running) {
        // 处理命令队列
        lv_mutex_lock(&g_cmd_mutex);
        if (g_cmd_count > 0) {
            Command cmd = g_cmd_queue[g_cmd_head];
            g_cmd_head = (g_cmd_head + 1) % CMD_QUEUE_SIZE;
            g_cmd_count--;
            lv_mutex_unlock(&g_cmd_mutex);
            
            // 模拟发送硬件命令
            printf("Device: send command to light %d, cmd=%d, value=%d\n", 
                   cmd.device_id, cmd.cmd, cmd.value);
            
            // 模拟硬件响应，触发状态变化事件
            LightModel* model = NULL;
            for (int i = 0; i < dm->light_count; i++) {
                if (dm->lights[i]->id == cmd.device_id) {
                    model = dm->lights[i];
                    break;
                }
            }
            
            if (model && model->is_valid) {
                if (cmd.cmd == 0) { // 开关命令
                    printf("DeviceManager: updating light %d is_on %d->%d\n", 
                           model->id, model->is_on, cmd.value);
                    model->is_on = cmd.value;
                } else if (cmd.cmd == 1) { // 亮度命令
                    printf("DeviceManager: updating light %d brightness %d->%d\n", 
                           model->id, model->brightness, cmd.value);
                    model->brightness = cmd.value;
                }
                
                // 触发事件
                Event ev;
                ev.type = EVENT_LIGHT_STATE_CHANGED;
                ev.data.light.device_id = model->id;
                ev.data.light.is_on = model->is_on;
                ev.data.light.brightness = model->brightness;
                event_bus_trigger(&ev);
            } else {
                printf("DeviceManager: model not found or invalid for device_id=%d\n", cmd.device_id);
            }
        } else {
            lv_mutex_unlock(&g_cmd_mutex);
        }
        
        lv_sleep_ms(100);
    }
    printf("DeviceManager: device thread exiting dm=%p\n", dm);
}

DeviceManager* device_manager_create(void) {
    DeviceManager* dm = (DeviceManager*)calloc(1, sizeof(DeviceManager));
    if (!dm) return NULL;
    
    printf("DeviceManager: creating device manager %p\n", dm);
    lv_mutex_init(&g_cmd_mutex);
    
    // 创建3个灯模型
    for (int i = 0; i < 3; i++) {
        LightModel* model = (LightModel*)malloc(sizeof(LightModel));
        model->id = i + 1;
        sprintf(model->name, "灯%d", i + 1);
        model->is_on = 0;
        model->brightness = 50;
        model->ref_count = 0;
        model->is_valid = 1;
        
        dm->lights[i] = model;
        printf("DeviceManager: created light model id=%d name='%s'\n", model->id, model->name);
    }
    dm->light_count = 3;
    dm->running = 0;
    
    printf("DeviceManager: created with %d lights\n", dm->light_count);
    return dm;
}

void device_manager_destroy(DeviceManager* dm) {
    if (!dm) {
        printf("DeviceManager: destroy called with NULL\n");
        return;
    }
    
    printf("DeviceManager: destroying device manager %p\n", dm);
    device_manager_stop(dm);
    
    for (int i = 0; i < dm->light_count; i++) {
        printf("DeviceManager: freeing light model id=%d\n", dm->lights[i]->id);
        free(dm->lights[i]);
    }
    
    free(dm);
    printf("DeviceManager: destroyed\n");
}

void device_manager_start(DeviceManager* dm) {
    if (!dm || dm->running) {
        printf("DeviceManager: start failed, dm=%p, running=%d\n", dm, dm ? dm->running : -1);
        return;
    }

    printf("DeviceManager: starting device thread\n");
    dm->running = 1;
    lv_thread_init(&dm->thread, "dev_thread", LV_THREAD_PRIO_LOW,
                   device_thread_entry, 4096, dm);
    printf("DeviceManager: device thread started\n");
}

void device_manager_stop(DeviceManager* dm) {
    if (!dm || !dm->running) {
        printf("DeviceManager: stop failed, dm=%p, running=%d\n", dm, dm ? dm->running : -1);
        return;
    }

    printf("DeviceManager: stopping device thread\n");
    dm->running = 0;
    lv_thread_delete(&dm->thread);
    printf("DeviceManager: device thread stopped\n");
}

LightModel* device_manager_get_light(DeviceManager* dm, int id) {
    if (!dm) {
        printf("DeviceManager: get_light called with NULL dm\n");
        return NULL;
    }
    
    for (int i = 0; i < dm->light_count; i++) {
        if (dm->lights[i]->id == id && dm->lights[i]->is_valid) {
            printf("DeviceManager: get_light id=%d found model=%p (valid=%d)\n", 
                   id, dm->lights[i], dm->lights[i]->is_valid);
            return dm->lights[i];
        }
    }
    printf("DeviceManager: get_light id=%d not found\n", id);
    return NULL;
}

void device_manager_send_command(DeviceManager* dm, int device_id, int cmd, int value) {
    (void)dm;
    
    printf("DeviceManager: send command device_id=%d, cmd=%d, value=%d\n", device_id, cmd, value);
    
    lv_mutex_lock(&g_cmd_mutex);
    
    if (g_cmd_count < CMD_QUEUE_SIZE) {
        Command* c = &g_cmd_queue[g_cmd_tail];
        c->device_id = device_id;
        c->cmd = cmd;
        c->value = value;
        
        g_cmd_tail = (g_cmd_tail + 1) % CMD_QUEUE_SIZE;
        g_cmd_count++;
        printf("DeviceManager: command queued, cmd_count=%d\n", g_cmd_count);
    } else {
        printf("DeviceManager: command queue full, dropping command\n");
    }
    
    lv_mutex_unlock(&g_cmd_mutex);
}