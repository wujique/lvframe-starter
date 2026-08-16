/**
 * @file         slots.h
 * @brief        box86 双槽实例声明：g_ui_slot（UI 线程消费）、g_dev_slot（DEV 线程消费）
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef BOX86_SLOTS_H
#define BOX86_SLOTS_H

#include "lvframe/slot.h"

#ifdef __cplusplus
extern "C" {
#endif

/** DEV → UI；UI 线程 50ms 定时器消费 */
extern lv_slot_t g_ui_slot;

/** UI → DEV；DEV 线程消费 */
extern lv_slot_t g_dev_slot;

/** 初始化两个槽实例 */
void box86_slots_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BOX86_SLOTS_H */
