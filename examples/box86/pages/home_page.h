/**
 * @file         home_page.h
 * @brief        主页接口：下拉设置面板 + 水平滑动设备页切换
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include "lvframe/page.h"
#include "models/model_store.h"

/**
 * @brief        主页初始化参数
 */
typedef struct {
    model_store_t* store; /**< 模型仓库 */
} HomePageParams;

/**
 * @brief        主页工厂函数
 */
Page* home_page_creator(void* params);

/**
 * @brief        重置主页到初始状态：关闭设置面板并切换到第一个设备页
 */
void home_page_reset_to_first(Page* home_page);

#endif /* HOME_PAGE_H */
