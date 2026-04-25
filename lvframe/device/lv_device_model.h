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

#ifdef __cplusplus
}
#endif

#endif /* LV_DEVICE_MODEL_H */
