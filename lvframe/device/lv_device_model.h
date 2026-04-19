#ifndef LV_DEVICE_MODEL_H
#define LV_DEVICE_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 配置参数 - 可在编译时覆盖 */
#ifndef LV_DEVICE_NAME_MAX
#define LV_DEVICE_NAME_MAX 32
#endif

#ifndef LV_MAX_DEVICES
#define LV_MAX_DEVICES 20
#endif

/* ── 公共基础模型（必须是所有设备结构体的第一个成员） ── */
typedef struct {
    int  id;                          /* 设备唯一ID */
    int  type;                        /* 设备类型（由应用层定义枚举） */
    char name[LV_DEVICE_NAME_MAX];    /* 设备名称 */
} lv_device_base_t;

/* ── 系统模型（全局设置，由 lv_device_store_t 持有） ── */
typedef struct {
    int brightness;       /* 屏幕亮度 0~100 */
    int volume;           /* 音量 0~100 */
    int network_enabled;  /* 网络开关 0=关 1=开 */
} lv_system_model_t;

#ifdef __cplusplus
}
#endif

#endif /* LV_DEVICE_MODEL_H */
