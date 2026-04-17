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

/* ── 设备类型枚举 ── */
typedef enum {
    LV_DEVICE_TYPE_LIGHT   = 0,   /* 普通灯 */
    LV_DEVICE_TYPE_CCT     = 1,   /* 色温灯 */
    LV_DEVICE_TYPE_CURTAIN = 2,   /* 电动窗帘 */
    LV_DEVICE_TYPE_CUSTOM_START = 100, /* 自定义设备类型从此开始 */
} lv_device_type_t;

/* ── 公共基础模型（必须是所有设备结构体的第一个成员） ── */
typedef struct {
    int              id;           /* 设备唯一ID */
    lv_device_type_t type;         /* 设备类型 */
    char             name[LV_DEVICE_NAME_MAX]; /* 设备名称 */
} lv_device_base_t;

/* ── 普通灯模型 ── */
typedef struct {
    lv_device_base_t base;         /* 必须第一个 */
    int              onoffsta;     /* 0=关 1=开 */
} lv_light_model_t;

/* ── 色温灯模型（继承普通灯） ── */
typedef struct {
    lv_device_base_t base;         /* 必须第一个 */
    int              onoffsta;     /* 0=关 1=开 */
    int              color_temp;   /* 2700~6500K */
} lv_cct_light_model_t;

/* ── 窗帘指令枚举 ── */
typedef enum {
    LV_CURTAIN_CMD_NONE  = 0,
    LV_CURTAIN_CMD_OPEN  = 1,
    LV_CURTAIN_CMD_CLOSE = 2,
    LV_CURTAIN_CMD_STOP  = 3,
} lv_curtain_cmd_t;

/* ── 电动窗帘模型 ── */
typedef struct {
    lv_device_base_t base;         /* 必须第一个 */
    int              position;     /* 0~100% */
    lv_curtain_cmd_t command;      /* 最后一次指令 */
} lv_curtain_model_t;

/* ── 系统模型（全局设置） ── */
typedef struct {
    int brightness;                /* 屏幕亮度 0~100 */
    int volume;                    /* 音量 0~100 */
    int network_enabled;           /* 网络开关 0=关 1=开 */
} lv_system_model_t;

/* ── 设备快照（用于UI线程安全读取） ── */
typedef struct {
    lv_device_base_t base;         /* 设备基础信息 */
    union {
        lv_light_model_t      light;
        lv_cct_light_model_t  cct_light;
        lv_curtain_model_t    curtain;
    } data;                        /* 具体设备数据 */
} lv_device_snapshot_t;

#ifdef __cplusplus
}
#endif

#endif /* LV_DEVICE_MODEL_H */