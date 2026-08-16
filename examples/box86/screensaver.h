/**
 * @file         screensaver.h
 * @brief        屏保状态机对外接口，提供初始化和唤醒功能
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */
#ifndef BOX86_SCREENSAVER_H
#define BOX86_SCREENSAVER_H

#include "models/model_store.h"

/**
 * @brief        初始化屏保状态机并启动 1s LVGL 定时器
 *
 * @param        store                模型仓库指针（用于读取屏保系统配置）
 * @return       void
 */
void screensaver_init(model_store_t* store);

/**
 * @brief        唤醒屏保：从息屏或屏保状态恢复到正常状态
 *
 * @return       void
 */
void screensaver_wake(void);

#endif
