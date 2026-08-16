#ifndef BOX86_BLANK_PAGE_H
#define BOX86_BLANK_PAGE_H

/**
 * @file         blank_page.h
 * @brief        息屏页接口：纯黑全屏，点击任意处唤醒
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "lvframe/page.h"

/**
 * @brief        供 page_manager 注册使用的息屏页工厂函数
 *
 * @param        params               model_store_t* 模型仓库（存入 user_data）
 * @return       Page* 新创建的息屏页对象
 */
Page* blank_page_creator(void* params);

#endif
