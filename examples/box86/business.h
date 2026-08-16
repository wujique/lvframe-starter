/**
 * @file         business.h
 * @brief        业务逻辑模块：消费 g_dev_slot、解析 Shell，经 g_ui_slot 通知 UI
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef BUSINESS_H
#define BUSINESS_H

#include "slots.h"
#include "models/model_store.h"

/**
 * @brief        业务逻辑上下文
 */
typedef struct {
    lv_slot_t*      dev_slot;  /**< g_dev_slot：UI → DEV，本线程消费 */
    lv_slot_t*      ui_slot;   /**< g_ui_slot：DEV → UI，本线程发送 */
    model_store_t*  store;     /**< 模型仓库 */
    int             running;   /**< 线程运行标志：1 运行，0 停止 */
    lv_thread_t     thread;    /**< 业务逻辑线程 */
} Business;

void business_init(Business* biz, lv_slot_t* dev_slot, lv_slot_t* ui_slot, model_store_t* store);
void business_start(Business* biz);
void business_stop(Business* biz);

#endif /* BUSINESS_H */
