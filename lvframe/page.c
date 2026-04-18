#include "page.h"
#include <stdlib.h>
#include <stdio.h>

static void (*s_root_created_cb)(lv_obj_t *root) = NULL;

void page_set_root_created_cb(void (*cb)(lv_obj_t *root))
{
    s_root_created_cb = cb;
}

Page* page_create(PageLifecycle* lifecycle, void* params) {
    Page* page = (Page*)calloc(1, sizeof(Page));
    if (!page) return NULL;
    
    page->root = lv_obj_create(lv_scr_act());
    lv_obj_add_flag(page->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(page->root, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(page->root, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(page->root, 0, LV_PART_MAIN);

    /* 通知应用层：root 已创建（可用于设置全局字体等） */
    if (s_root_created_cb) s_root_created_cb(page->root);
    
    if (lifecycle) {
        page->lifecycle = *lifecycle;
    }
    
    page->state = PAGE_STATE_CREATED;
    page->model_valid = 1;
    
    printf("Page: created page=%p, root=%p, state=CREATED\n", page, page->root);
    
    if (page->lifecycle.on_create) {
        page->lifecycle.on_create(page, params);
    }
    
    return page;
}

void page_destroy(Page* page) {
    if (!page) return;
    
    printf("Page: destroying page=%p, root=%p, state=%d\n", page, page->root, page->state);
    
    if (page->lifecycle.on_destroy) {
        page->lifecycle.on_destroy(page);
    }
    
    if (page->root) {
        lv_obj_delete(page->root);
    }
    
    free(page);
}

lv_obj_t* page_get_root(Page* page) {
    return page ? page->root : NULL;
}

void page_set_user_data(Page* page, void* data) {
    if (page) page->user_data = data;
}

void* page_get_user_data(Page* page) {
    return page ? page->user_data : NULL;
}

void page_set_model_valid(Page* page, int valid) {
    if (page) {
        printf("Page: set model valid page=%p, valid=%d (was %d)\n", page, valid, page->model_valid);
        page->model_valid = valid;
    }
}

int page_is_model_valid(Page* page) {
    return page ? page->model_valid : 0;
}
