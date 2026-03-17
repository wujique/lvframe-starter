#include "page.h"
#include <stdlib.h>

Page* page_create(PageLifecycle* lifecycle, void* params) {
    Page* page = (Page*)calloc(1, sizeof(Page));
    if (!page) return NULL;
    
    page->root = lv_obj_create(lv_scr_act());
    lv_obj_add_flag(page->root, LV_OBJ_FLAG_HIDDEN);
    
    if (lifecycle) {
        page->lifecycle = *lifecycle;
    }
    
    page->state = PAGE_STATE_CREATED;
    page->model_valid = 1;
    
    if (page->lifecycle.on_create) {
        page->lifecycle.on_create(page, params);
    }
    
    return page;
}

void page_destroy(Page* page) {
    if (!page) return;
    
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
    if (page) page->model_valid = valid;
}

int page_is_model_valid(Page* page) {
    return page ? page->model_valid : 0;
}
