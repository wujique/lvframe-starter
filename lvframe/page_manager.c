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
    printf("PageManager: initialized\n");
}

void page_manager_set_cache_size(int size) {
    if (size > MAX_CACHE_SIZE) size = MAX_CACHE_SIZE;
    g_manager.cache_size = size;
}

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

static PageCreator find_creator(const char* name) {
    for (int i = 0; i < g_manager.registry_count; i++) {
        if (strcmp(g_manager.registry[i].name, name) == 0) {
            return g_manager.registry[i].creator;
        }
    }
    return NULL;
}

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

void page_manager_back_to_home(void) {
    printf("PageManager: back to home, stack_top=%d\n", g_manager.stack_top);
    while (g_manager.stack_top > 0) {
        page_manager_back();
    }
    printf("PageManager: now at home page\n");
}

Page* page_manager_get_current(void) {
    if (g_manager.stack_top >= 0) {
        return g_manager.stack[g_manager.stack_top].page;
    }
    return NULL;
}

void page_manager_handle_back_key(void) {
    page_manager_back();
}

Page* page_manager_find_page_by_model(void* model) {
    printf("PageManager: find page by model=%p\n", model);
    for (int i = 0; i <= g_manager.stack_top; i++) {
        if (g_manager.stack[i].page && g_manager.stack[i].page->user_data == model) {
            printf("PageManager: found in stack index=%d, page=%p\n", i, g_manager.stack[i].page);
            return g_manager.stack[i].page;
        }
    }

    for (int i = 0; i < g_manager.cache_count; i++) {
        if (g_manager.cache[i].page && g_manager.cache[i].page->user_data == model) {
            printf("PageManager: found in cache index=%d, page=%p\n", i, g_manager.cache[i].page);
            return g_manager.cache[i].page;
        }
    }

    printf("PageManager: model not found\n");
    return NULL;
}
