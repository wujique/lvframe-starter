/**
 * @file         slots.c
 * @brief        box86 双槽实例定义与初始化
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#include "slots.h"

lv_slot_t g_ui_slot;   /**< DEV → UI；UI 线程 50ms 定时器消费 */
lv_slot_t g_dev_slot;  /**< UI → DEV；DEV 线程消费 */

void box86_slots_init(void)
{
    lv_slot_init(&g_ui_slot, "g_ui_slot");
    lv_slot_init(&g_dev_slot, "g_dev_slot");
}
