#ifndef BOX86_SCREENSAVER_PAGE_H
#define BOX86_SCREENSAVER_PAGE_H

/**
 * @file         screensaver_page.h
 * @brief        屏保展示页接口：黑底白字，点击任意处唤醒
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "lvframe/page.h"

/**
 * @brief        供 page_manager 注册使用的屏保页工厂函数
 *
 * @param        params               保留参数（未使用）
 * @return       Page* 新创建的屏保页对象
 */
Page* screensaver_page_creator(void* params);

#endif
