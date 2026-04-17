#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "page.h"

/* 错误码 */
typedef enum {
    PAGE_MANAGER_OK = 0,
    PAGE_MANAGER_ERR_NOT_FOUND,      /* 页面未注册 */
    PAGE_MANAGER_ERR_STACK_FULL,     /* 页面栈满 */
    PAGE_MANAGER_ERR_NULL_POINTER,   /* 空指针参数 */
    PAGE_MANAGER_ERR_INTERNAL,       /* 内部错误 */
} PageManagerError;

#define MAX_PAGE_STACK 20
#define MAX_CACHE_SIZE 10
#define MAX_PAGE_NAME 32

typedef Page* (*PageCreator)(void* params);

typedef struct {
    char name[MAX_PAGE_NAME];
    PageCreator creator;
} PageRegistry;

typedef struct {
    char name[MAX_PAGE_NAME];
    Page* page;
} CachedPage;

typedef struct {
    char name[MAX_PAGE_NAME];      /* 页面在栈中对应的注册名称 */
    Page* page;
} StackEntry;

typedef struct {
    StackEntry stack[MAX_PAGE_STACK];
    int stack_top;
    CachedPage cache[MAX_CACHE_SIZE];
    int cache_size;
    int cache_count;
    PageRegistry registry[20];
    int registry_count;
} PageManager;

void page_manager_init(void);
void page_manager_set_cache_size(int size);
void page_manager_register(const char* name, PageCreator creator);
int page_manager_open(const char* name, void* params);
void page_manager_back(void);
void page_manager_back_to_home(void);
Page* page_manager_get_current(void);
void page_manager_handle_back_key(void);
Page* page_manager_find_page_by_model(void* model);

#endif