/**
 * device_model.h - box86 设备数据模型定义
 *
 * 此文件属于 box86 应用层，定义智能家居中控屏的具体设备类型和数据结构。
 * lvframe 框架只提供通用的 lv_device_base_t 基础结构和存储机制，
 * 具体设备类型（灯、色温灯、窗帘等）在此处定义。
 */

#ifndef BOX86_DEVICE_MODEL_H
#define BOX86_DEVICE_MODEL_H

#include "lvframe/device/lv_device_store.h"  /* 获取 lv_device_base_t、LV_DEVICE_NAME_MAX */

#ifdef __cplusplus
extern "C" {
#endif

/* ── 设备类型枚举（box86 专用） ── */
typedef enum {
    BOX86_DEVICE_TYPE_LIGHT   = 0,   /* 普通灯 */
    BOX86_DEVICE_TYPE_CCT     = 1,   /* 色温灯 */
    BOX86_DEVICE_TYPE_CURTAIN = 2,   /* 电动窗帘 */
} box86_device_type_t;

/* ── 普通灯模型 ── */
typedef struct {
    lv_device_base_t base;     /* 必须是第一个成员 */
    int              onoffsta; /* 0=关 1=开 */
} box86_light_model_t;

/* ── 色温灯模型 ── */
typedef struct {
    lv_device_base_t base;       /* 必须是第一个成员 */
    int              onoffsta;   /* 0=关 1=开 */
    int              color_temp; /* 2700~6500K */
} box86_cct_light_model_t;

/* ── 窗帘指令枚举 ── */
typedef enum {
    BOX86_CURTAIN_CMD_NONE  = 0,
    BOX86_CURTAIN_CMD_OPEN  = 1,
    BOX86_CURTAIN_CMD_CLOSE = 2,
    BOX86_CURTAIN_CMD_STOP  = 3,
} box86_curtain_cmd_t;

/* ── 电动窗帘模型 ── */
typedef struct {
    lv_device_base_t    base;    /* 必须是第一个成员 */
    int                 position; /* 0~100% */
    box86_curtain_cmd_t command;  /* 最后一次指令 */
} box86_curtain_model_t;

/* ── 系统模型（全局设置） ── */
typedef struct {
    int brightness;      /* 屏幕亮度 0~100 */
    int volume;          /* 音量 0~100 */
    int network_enabled; /* 网络开关 0=关 1=开 */
} box86_system_model_t;

#ifdef __cplusplus
}
#endif

#endif /* BOX86_DEVICE_MODEL_H */
