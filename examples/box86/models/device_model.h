#ifndef DEVICE_MODEL_H
#define DEVICE_MODEL_H

#include <stdint.h>

#define DEVICE_NAME_MAX  32
#define MAX_DEVICES      20

/* ── 设备类型 ── */
typedef enum {
    DEVICE_TYPE_LIGHT   = 0,
    DEVICE_TYPE_CCT     = 1,
    DEVICE_TYPE_CURTAIN = 2,
} DeviceType;

/* ── 公共基础模型（必须是所有设备结构体的第一个成员） ── */
typedef struct {
    int        id;
    DeviceType type;
    char       name[DEVICE_NAME_MAX];
} DeviceBase;

/* ── 普通灯 ── */
typedef struct {
    DeviceBase base;       /* 必须第一个 */
    int        onoffsta;   /* 0=关 1=开 */
} LightModel;

/* ── 色温灯（继承普通灯，base 字段完全相同） ── */
typedef struct {
    DeviceBase base;       /* 必须第一个 */
    int        onoffsta;   /* 0=关 1=开 */
    int        color_temp; /* 2700~6500 */
} CctLightModel;

/* ── 窗帘指令 ── */
typedef enum {
    CURTAIN_CMD_NONE  = 0,
    CURTAIN_CMD_OPEN  = 1,
    CURTAIN_CMD_CLOSE = 2,
    CURTAIN_CMD_STOP  = 3,
} CurtainCmd;

/* ── 电动窗帘 ── */
typedef struct {
    DeviceBase base;       /* 必须第一个 */
    int        position;   /* 0~100 */
    CurtainCmd command;    /* 最后一次指令 */
} CurtainModel;

/* ── 系统模型 ── */
typedef struct {
    int brightness;      /* 0~100 */
    int volume;          /* 0~100 */
    int network_enabled; /* 0=关 1=开 */
} SystemModel;

#endif /* DEVICE_MODEL_H */
