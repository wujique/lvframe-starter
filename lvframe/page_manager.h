/**
 * @file         page_manager.h
 * @brief        页面管理器，提供页面注册、栈式导航和 LRU 缓存能力
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "page.h"

/**
 * @brief        页面管理器操作错误码
 */
typedef enum {
    PAGE_MANAGER_OK = 0,
    PAGE_MANAGER_ERR_NOT_FOUND,      /**< 页面未注册 */
    PAGE_MANAGER_ERR_STACK_FULL,     /**< 页面栈满 */
    PAGE_MANAGER_ERR_NULL_POINTER,   /**< 空指针参数 */
    PAGE_MANAGER_ERR_INTERNAL,       /**< 内部错误 */
} PageManagerError;

/** 页面导航栈最大深度 */
#define MAX_PAGE_STACK 20
/** 页面缓存最大容量 */
#define MAX_CACHE_SIZE 10
/** 页面注册名称最大长度（含终止符） */
#define MAX_PAGE_NAME 32

/**
 * @brief        页面创建工厂函数类型，由 page_manager_register 注册
 *
 * @param        params               创建参数，由调用方传入
 * @return       Page* 新页面指针
 */
typedef Page* (*PageCreator)(void* params);

/**
 * @brief        页面注册表条目，关联名称与工厂函数
 */
typedef struct {
    char name[MAX_PAGE_NAME]; /**< 注册名称 */
    PageCreator creator;      /**< 对应的工厂函数 */
} PageRegistry;

/**
 * @brief        缓存条目，保存暂时不在栈中的页面供复用
 */
typedef struct {
    char name[MAX_PAGE_NAME]; /**< 页面注册名称 */
    Page* page;               /**< 已创建但暂缓的页面指针 */
} CachedPage;

/**
 * @brief        导航栈条目，保存当前栈中页面及其名称
 */
typedef struct {
    char name[MAX_PAGE_NAME]; /**< 页面在栈中对应的注册名称 */
    Page* page;               /**< 页面指针 */
} StackEntry;

/**
 * @brief        页面管理器内部状态
 */
typedef struct {
    StackEntry stack[MAX_PAGE_STACK];  /**< 导航栈 */
    int stack_top;                     /**< 栈顶索引，-1 表示空栈 */
    CachedPage cache[MAX_CACHE_SIZE];  /**< LRU 页面缓存 */
    int cache_size;                    /**< 当前允许的最大缓存数量 */
    int cache_count;                   /**< 当前已缓存的页面数量 */
    PageRegistry registry[20];         /**< 页面注册表 */
    int registry_count;                /**< 已注册页面数量 */
} PageManager;

/**
 * @brief        初始化页面管理器，清空栈、缓存和注册表
 *
 * @return       void
 */
void page_manager_init(void);

/**
 * @brief        设置缓存大小上限（不超过 MAX_CACHE_SIZE）
 *
 * @param        size                 期望的缓存条目数
 * @return       void
 */
void page_manager_set_cache_size(int size);

/**
 * @brief        向注册表中注册页面名称和工厂函数
 *
 * 若同名页面已存在，则更新其工厂函数。
 *
 * @param        name                 页面唯一注册名称
 * @param        creator              页面工厂函数
 * @return       void
 */
void page_manager_register(const char* name, PageCreator creator);

/**
 * @brief        打开指定名称的页面，优先从缓存恢复，否则通过工厂创建
 *
 * 当前页面依次调用 on_pause / on_stop 后入后台，新页面调用
 * on_start / on_resume 后显示。
 *
 * @param        name                 已注册的页面名称
 * @param        params               透传给工厂函数或 on_start 的参数
 * @return       int PageManagerError 错误码，成功返回 PAGE_MANAGER_OK
 */
int page_manager_open(const char* name, void* params);

/**
 * @brief        返回上一个页面，当前页面缓存
 *
 * 若已处于首页则忽略。
 *
 * @return       void
 */
void page_manager_back(void);

/**
 * @brief        一直返回直到栈中只剩首页
 *
 * @return       void
 */
void page_manager_back_to_home(void);

/**
 * @brief        获取当前栈顶页面指针
 *
 * @return       Page* 当前页面指针，栈为空时返回 NULL
 */
Page* page_manager_get_current(void);

/**
 * @brief        处理返回键事件，等同于 page_manager_back
 *
 * @return       void
 */
void page_manager_handle_back_key(void);

#endif
