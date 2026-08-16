/**
 * @file         device_model.h
 * @brief        box86 应用层具体设备模型定义：灯 / 色温灯 / 电动窗帘 / 系统设置
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef BOX86_DEVICE_MODEL_H
#define BOX86_DEVICE_MODEL_H

#include "model_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief        普通灯设备模型（base 必须是第一个成员）
 */
typedef struct {
    model_base_t base;     /**< 通用模型头 */
    int          onoffsta; /**< 开关状态：0 = 关，1 = 开 */
} box86_light_model_t;

/**
 * @brief        色温灯设备模型（base 必须是第一个成员）
 */
typedef struct {
    model_base_t base;       /**< 通用模型头 */
    int          onoffsta;   /**< 开关状态：0 = 关，1 = 开 */
    int          color_temp; /**< 色温值，范围 2700~6500K */
} box86_cct_light_model_t;

/**
 * @brief        窗帘控制指令枚举
 */
typedef enum {
    BOX86_CURTAIN_CMD_NONE  = 0, /**< 无指令 */
    BOX86_CURTAIN_CMD_OPEN  = 1, /**< 打开 */
    BOX86_CURTAIN_CMD_CLOSE = 2, /**< 关闭 */
    BOX86_CURTAIN_CMD_STOP  = 3, /**< 停止 */
} box86_curtain_cmd_t;

/**
 * @brief        电动窗帘设备模型（base 必须是第一个成员）
 */
typedef struct {
    model_base_t        base;     /**< 通用模型头 */
    int                 position; /**< 开合位置，0~100% */
    box86_curtain_cmd_t command;  /**< 最后一次控制指令 */
} box86_curtain_model_t;

/**
 * @brief        系统设置模型（虚拟模型，base 必须是第一个成员）
 *
 * id 固定为 0，type = MODEL_TYPE_SYSTEM，常驻仓库。
 */
typedef struct {
    model_base_t base;                 /**< 通用模型头（id=0, type=SYSTEM）*/
    int brightness;                    /**< 屏幕亮度，0~100 */
    int volume;                        /**< 音量，0~100 */
    int network_enabled;               /**< 网络开关：0 = 关，1 = 开 */
    int screensaver_enabled;           /**< 屏保开关：0 = 关，1 = 开 */
    int screensaver_timeout;           /**< 无操作待机时间（秒）*/
    int screensaver_duration;          /**< 屏保持续时间（秒）*/
    int wake_action;                   /**< 息屏唤醒后行为：0 = 屏保页，1 = 首页 */
} box86_system_model_t;

#ifdef __cplusplus
}
#endif

#endif /* BOX86_DEVICE_MODEL_H */
