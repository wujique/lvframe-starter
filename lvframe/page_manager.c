#include "page_manager.h"
#include <string.h>
#include <stdio.h>

static PageManager g_manager;

void page_manager_init(void) {
    g_manager.stack_top = -1;
    g_manager.cache_size = 3;
    g_manager.cache_count = 0;
    g_manager.registry_count = 0;
    memset(g_manager.stack, 0, sizeof(g_manager.stack));
    memset(g_manager.cache, 0, sizeof(g_manager.cache));
}

void page_manager_set_cache_size(int size) {
    if (size > MAX_CACHE_SIZE) size = MAX_CACHE_SIZE;
    g_manager.cache_size = size;
}

void page_manager_register(const char* name, PageCreator creator) {
    if (g_manager.registry_count >= 20) return;
    
    PageRegistry* reg = &g_manager.registry[g_manager.registry_count++];
    strncpy(reg->name, name, MAX_PAGE_NAME - 1);
    reg->creator = creator;
}

static PageCreator find_creator(const char* name) {
    for (int i = 0; i < g_manager.registry_count; i++) {
        if (strcmp(g_manager.registry[i].name, name) == 0) {
            return g_manager.registry[i].creator;
        }
    }
    return NULL;
}

static void add_to_cache(Page* page) {
    if (!page) return;
    
    if (g_manager.cache_count < g_manager.cache_size) {
        g_manager.cache[g_manager.cache_count++] = page;
    } else {
        // 简单的FIFO策略
        Page* oldest = g_manager.cache[0];
        if (oldest) {
            page_destroy(oldest);
        }
        memmove(&g_manager.cache[0], &g_manager.cache[1], 
                (g_manager.cache_size - 1) * sizeof(Page*));
        g_manager.cache[g_manager.cache_size - 1] = page;
    }
}

static Page* find_in_cache(const char* name) {
    // 简化实现，实际应该根据参数判断
    return NULL;
}

void page_manager_open(const char* name, void* params) {
    Page* current = NULL;
    
    // 暂停当前页面
    if (g_manager.stack_top >= 0) {
        current = g_manager.stack[g_manager.stack_top];
        if (current->lifecycle.on_pause) {
            current->lifecycle.on_pause(current);
        }
        current->state = PAGE_STATE_PAUSED;
        lv_obj_add_flag(current->root, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 创建新页面
    PageCreator creator = find_creator(name);
    if (!creator) {
        printf("Page %s not registered\n", name);
        return;
    }
    
    Page* new_page = creator(params);
    if (!new_page) return;
    
    // 显示新页面
    lv_obj_remove_flag(new_page->root, LV_OBJ_FLAG_HIDDEN);
    
    // 启动新页面
    if (new_page->state == PAGE_STATE_CREATED) {
        if (new_page->lifecycle.on_start) {
            new_page->lifecycle.on_start(new_page);
        }
        new_page->state = PAGE_STATE_STARTED;
    }
    
    if (new_page->lifecycle.on_resume) {
        new_page->lifecycle.on_resume(new_page);
    }
    new_page->state = PAGE_STATE_RESUMED;
    
    // 入栈
    g_manager.stack[++g_manager.stack_top] = new_page;
    
    // 停止当前页面
    if (current && current->lifecycle.on_stop) {
        current->lifecycle.on_stop(current);
        current->state = PAGE_STATE_STOPPED;
    }
    
    lv_refr_now(lv_display_get_default());
}

void page_manager_back(void) {
    if (g_manager.stack_top <= 0) {
        // 首页，退出应用或忽略
        return;
    }
    
    Page* current = g_manager.stack[g_manager.stack_top--];
    Page* previous = g_manager.stack[g_manager.stack_top];
    
    // 暂停当前页面
    if (current->lifecycle.on_pause) {
        current->lifecycle.on_pause(current);
    }
    current->state = PAGE_STATE_PAUSED;
    
    // 停止当前页面
    if (current->lifecycle.on_stop) {
        current->lifecycle.on_stop(current);
    }
    current->state = PAGE_STATE_STOPPED;
    
    // 隐藏当前页面
    lv_obj_add_flag(current->root, LV_OBJ_FLAG_HIDDEN);
    
    // 恢复上一页面
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
    
    // 缓存或销毁当前页面
    add_to_cache(current);

    lv_refr_now(lv_display_get_default());
}

void page_manager_back_to_home(void) {
    while (g_manager.stack_top > 0) {
        page_manager_back();
    }
}

Page* page_manager_get_current(void) {
    if (g_manager.stack_top >= 0) {
        return g_manager.stack[g_manager.stack_top];
    }
    return NULL;
}

void page_manager_handle_back_key(void) {
    page_manager_back();
}

Page* page_manager_find_page_by_model(void* model) {
    // 遍历栈和缓存查找引用该模型的页面
    for (int i = 0; i <= g_manager.stack_top; i++) {
        if (g_manager.stack[i]->user_data == model) {
            return g_manager.stack[i];
        }
    }
    
    for (int i = 0; i < g_manager.cache_count; i++) {
        if (g_manager.cache[i]->user_data == model) {
            return g_manager.cache[i];
        }
    }
    
    return NULL;
}
