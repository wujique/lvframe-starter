/**
 * @file         page_manager.c
 * @brief        页面管理器实现，包含栈式导航、LRU 缓存和注册表查找
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "page_manager.h"
#include <string.h>
#include <stdio.h>

/** 全局页面管理器实例 */
static PageManager g_manager;

/**
 * @brief        初始化页面管理器，清空栈、缓存和注册表
 *
 * @return       void
 */
void page_manager_init(void) {
    g_manager.stack_top = -1;
    g_manager.cache_size = 3;
    g_manager.cache_count = 0;
    g_manager.registry_count = 0;
    memset(g_manager.stack, 0, sizeof(g_manager.stack));
    memset(g_manager.cache, 0, sizeof(g_manager.cache));
    printf("PageManager: initialized\n");
}

/**
 * @brief        设置缓存大小上限（超过 MAX_CACHE_SIZE 时截断）
 *
 * @param        size                 期望的缓存条目数
 * @return       void
 */
void page_manager_set_cache_size(int size) {
    if (size > MAX_CACHE_SIZE) size = MAX_CACHE_SIZE;
    g_manager.cache_size = size;
}

/**
 * @brief        向注册表中注册页面名称和工厂函数
 *
 * 同名页面已存在时更新工厂函数而不重复添加。
 *
 * @param        name                 页面唯一注册名称
 * @param        creator              页面工厂函数
 * @return       void
 */
void page_manager_register(const char* name, PageCreator creator) {
    if (g_manager.registry_count >= 20) {
        printf("PageManager: registry full, cannot register page '%s'\n", name);
        return;
    }

    for (int i = 0; i < g_manager.registry_count; i++) {
        if (strcmp(g_manager.registry[i].name, name) == 0) {
            printf("PageManager: page '%s' already registered, updating creator\n", name);
            g_manager.registry[i].creator = creator;
            return;
        }
    }

    PageRegistry* reg = &g_manager.registry[g_manager.registry_count++];
    strncpy(reg->name, name, MAX_PAGE_NAME - 1);
    reg->name[MAX_PAGE_NAME - 1] = '\0';
    reg->creator = creator;
    printf("PageManager: registered page '%s' (total %d)\n", name, g_manager.registry_count);
}

/**
 * @brief        在注册表中按名称查找工厂函数
 *
 * @param        name                 要查找的页面名称
 * @return       PageCreator 工厂函数指针，未找到返回 NULL
 */
static PageCreator find_creator(const char* name) {
    for (int i = 0; i < g_manager.registry_count; i++) {
        if (strcmp(g_manager.registry[i].name, name) == 0) {
            return g_manager.registry[i].creator;
        }
    }
    return NULL;
}

/**
 * @brief        将页面加入 LRU 缓存，超出容量时淘汰最旧条目
 *
 * @param        name                 页面注册名称
 * @param        page                 要缓存的页面指针
 * @return       void
 */
static void add_to_cache(const char* name, Page* page) {
    if (!page) return;

    if (g_manager.cache_count < g_manager.cache_size) {
        CachedPage* cp = &g_manager.cache[g_manager.cache_count++];
        strncpy(cp->name, name, MAX_PAGE_NAME - 1);
        cp->name[MAX_PAGE_NAME - 1] = '\0';
        cp->page = page;
        printf("PageManager: cached page=%p name='%s', cache_count=%d\n", page, name, g_manager.cache_count);
    } else {
        CachedPage* oldest = &g_manager.cache[0];
        if (oldest->page) {
            printf("PageManager: cache full, destroying oldest page=%p name='%s'\n", oldest->page, oldest->name);
            page_destroy(oldest->page);
        }
        memmove(&g_manager.cache[0], &g_manager.cache[1],
                (g_manager.cache_size - 1) * sizeof(CachedPage));
        CachedPage* cp = &g_manager.cache[g_manager.cache_size - 1];
        strncpy(cp->name, name, MAX_PAGE_NAME - 1);
        cp->name[MAX_PAGE_NAME - 1] = '\0';
        cp->page = page;
        printf("PageManager: cached page=%p name='%s' (replaced oldest)\n", page, name);
    }
}

/**
 * @brief        在缓存中按名称查找并取出页面（命中后从缓存移除）
 *
 * @param        name                 要查找的页面名称
 * @return       Page* 命中的页面指针，未命中返回 NULL
 */
static Page* find_in_cache(const char* name) {
    for (int i = 0; i < g_manager.cache_count; i++) {
        if (strcmp(g_manager.cache[i].name, name) == 0) {
            Page* page = g_manager.cache[i].page;
            memmove(&g_manager.cache[i], &g_manager.cache[i + 1],
                    (g_manager.cache_count - i - 1) * sizeof(CachedPage));
            g_manager.cache_count--;
            memset(&g_manager.cache[g_manager.cache_count], 0, sizeof(CachedPage));
            printf("PageManager: found in cache page=%p name='%s'\n", page, name);
            return page;
        }
    }
    return NULL;
}

/**
 * @brief        打开指定名称的页面，优先从缓存恢复，否则通过工厂创建
 *
 * 当前页面依次执行 on_pause / on_stop，新页面执行 on_start / on_resume，
 * 打开失败时恢复当前页面到前台。
 *
 * @param        name                 已注册的页面名称
 * @param        params               透传给工厂函数或 on_start 的参数
 * @return       int PageManagerError 错误码，成功返回 PAGE_MANAGER_OK
 */
int page_manager_open(const char* name, void* params) {
    printf("PageManager: opening page '%s', params=%p\n", name, params);

    if (!name) {
        printf("PageManager: ERROR null pointer\n");
        return PAGE_MANAGER_ERR_NULL_POINTER;
    }

    if (g_manager.stack_top >= MAX_PAGE_STACK - 1) {
        printf("PageManager: ERROR stack full (top=%d)\n", g_manager.stack_top);
        return PAGE_MANAGER_ERR_STACK_FULL;
    }

    Page* current = NULL;

    if (g_manager.stack_top >= 0) {
        current = g_manager.stack[g_manager.stack_top].page;
        if (current->lifecycle.on_pause) {
            current->lifecycle.on_pause(current);
        }
        current->state = PAGE_STATE_PAUSED;
        lv_obj_add_flag(current->root, LV_OBJ_FLAG_HIDDEN);
    }

    Page* new_page = find_in_cache(name);
    if (!new_page) {
        /* 缓存中没找到，通过注册表创建新页面 */
        PageCreator creator = find_creator(name);
        if (!creator) {
            printf("PageManager: ERROR page '%s' not found in registry\n", name);
            if (current) {
                lv_obj_remove_flag(current->root, LV_OBJ_FLAG_HIDDEN);
                if (current->lifecycle.on_resume) {
                    current->lifecycle.on_resume(current);
                }
                current->state = PAGE_STATE_RESUMED;
            }
            return PAGE_MANAGER_ERR_NOT_FOUND;
        }

        new_page = creator(params);
        if (!new_page) {
            printf("PageManager: ERROR page creation failed for '%s'\n", name);
            if (current) {
                lv_obj_remove_flag(current->root, LV_OBJ_FLAG_HIDDEN);
                if (current->lifecycle.on_resume) {
                    current->lifecycle.on_resume(current);
                }
                current->state = PAGE_STATE_RESUMED;
            }
            return PAGE_MANAGER_ERR_INTERNAL;
        }
        printf("PageManager: created new page=%p, root=%p\n", new_page, new_page->root);
    } else {
        /* 从缓存恢复页面：不再调用 on_create，直接走 on_start/on_resume */
        printf("PageManager: reused cached page=%p, root=%p\n", new_page, new_page->root);
    }

    if (new_page->state == PAGE_STATE_CREATED || new_page->state == PAGE_STATE_STOPPED) {
        if (new_page->lifecycle.on_start) {
            new_page->lifecycle.on_start(new_page);
        }
        new_page->state = PAGE_STATE_STARTED;
    }

    if (new_page->lifecycle.on_resume) {
        new_page->lifecycle.on_resume(new_page);
    }
    new_page->state = PAGE_STATE_RESUMED;

    lv_obj_remove_flag(new_page->root, LV_OBJ_FLAG_HIDDEN);

    /* 将页面和名称一起入栈 */
    g_manager.stack_top++;
    strncpy(g_manager.stack[g_manager.stack_top].name, name, MAX_PAGE_NAME - 1);
    g_manager.stack[g_manager.stack_top].name[MAX_PAGE_NAME - 1] = '\0';
    g_manager.stack[g_manager.stack_top].page = new_page;
    printf("PageManager: page pushed to stack, new top=%d\n", g_manager.stack_top);

    if (current && current->lifecycle.on_stop) {
        current->lifecycle.on_stop(current);
        current->state = PAGE_STATE_STOPPED;
    }

    printf("PageManager: open '%s' completed successfully\n", name);
    return PAGE_MANAGER_OK;
}

/**
 * @brief        返回上一个页面，当前页面缓存以备复用
 *
 * 若已处于首页（stack_top <= 0）则忽略。
 *
 * @return       void
 */
void page_manager_back(void) {
    printf("PageManager: back triggered, stack_top=%d\n", g_manager.stack_top);
    if (g_manager.stack_top <= 0) {
        printf("PageManager: already at home page, ignoring\n");
        return;
    }

    /* 取出栈顶条目（包含名称和页面） */
    StackEntry cur_entry = g_manager.stack[g_manager.stack_top--];
    Page* current  = cur_entry.page;
    Page* previous = g_manager.stack[g_manager.stack_top].page;
    printf("PageManager: current page=%p name='%s', previous page=%p\n",
           current, cur_entry.name, previous);

    if (current->lifecycle.on_pause) {
        current->lifecycle.on_pause(current);
    }
    current->state = PAGE_STATE_PAUSED;

    if (current->lifecycle.on_stop) {
        current->lifecycle.on_stop(current);
    }
    current->state = PAGE_STATE_STOPPED;

    lv_obj_add_flag(current->root, LV_OBJ_FLAG_HIDDEN);

    if (previous->state == PAGE_STATE_STOPPED) {
        if (previous->lifecycle.on_start) {
            previous->lifecycle.on_start(previous);
        }
        previous->state = PAGE_STATE_STARTED;
    }

    if (previous->lifecycle.on_resume) {
        previous->lifecycle.on_resume(previous);
    }
    previous->state = PAGE_STATE_RESUMED;

    lv_obj_remove_flag(previous->root, LV_OBJ_FLAG_HIDDEN);

    /* 使用栈中保存的真实页面名称存入缓存 */
    add_to_cache(cur_entry.name, current);

    printf("PageManager: back completed, new stack_top=%d\n", g_manager.stack_top);
}

/**
 * @brief        循环调用 page_manager_back 直到栈中只剩首页
 *
 * @return       void
 */
void page_manager_back_to_home(void) {
    printf("PageManager: back to home, stack_top=%d\n", g_manager.stack_top);
    while (g_manager.stack_top > 0) {
        page_manager_back();
    }
    printf("PageManager: now at home page\n");
}

/**
 * @brief        获取当前栈顶页面指针
 *
 * @return       Page* 当前页面指针，栈为空时返回 NULL
 */
Page* page_manager_get_current(void) {
    if (g_manager.stack_top >= 0) {
        return g_manager.stack[g_manager.stack_top].page;
    }
    return NULL;
}

/**
 * @brief        处理返回键事件，等同于 page_manager_back
 *
 * @return       void
 */
void page_manager_handle_back_key(void) {
    page_manager_back();
}
