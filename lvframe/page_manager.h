#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "page.h"

#define MAX_PAGE_STACK 20
#define MAX_CACHE_SIZE 10
#define MAX_PAGE_NAME 32

typedef Page* (*PageCreator)(void* params);

typedef struct {
    char name[MAX_PAGE_NAME];
    PageCreator creator;
} PageRegistry;

typedef struct {
    Page* stack[MAX_PAGE_STACK];
    int stack_top;
    Page* cache[MAX_CACHE_SIZE];
    int cache_size;
    int cache_count;
    PageRegistry registry[20];
    int registry_count;
} PageManager;

void page_manager_init(void);
void page_manager_set_cache_size(int size);
void page_manager_register(const char* name, PageCreator creator);
void page_manager_open(const char* name, void* params);
void page_manager_back(void);
void page_manager_back_to_home(void);
Page* page_manager_get_current(void);
void page_manager_handle_back_key(void);
Page* page_manager_find_page_by_model(void* model);

#endif