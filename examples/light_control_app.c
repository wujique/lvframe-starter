#include "../lvframe/page_manager.h"
#include "../lvframe/swipe_container.h"
#include "../lvframe/event_bus.h"
#include "../lvframe/device_manager.h"
#include "../lvframe/impl/tileview_impl.c"

// 全局设备管理器
static DeviceManager* g_dm;

// 灯页面数据结构
typedef struct {
    LightModel* model;
    lv_obj_t* switch_btn;
    lv_obj_t* btn_label;
    lv_obj_t* slider;
    lv_obj_t* removed_label;
} LightPageData;

// 主页面数据结构
typedef struct {
    lv_obj_t* swipe_container;
} MainPageData;

// 函数声明
static Page* main_page_create(void* params);
static Page* light_page_create(void* params);

// 灯页面生命周期函数
static void light_page_on_create(Page* page, void* params) {
    LightModel* model = (LightModel*)params;
    if (!model || !model->is_valid) {
        page_set_model_valid(page, 0);
        return;
    }
    
    LightPageData* data = (LightPageData*)malloc(sizeof(LightPageData));
    memset(data, 0, sizeof(LightPageData));
    data->model = model;
    model->ref_count++;
    
    lv_obj_t* root = page_get_root(page);
    
    // 创建标题
    char title[32];
    sprintf(title, "%s 控制", model->name);
    lv_obj_t* label = lv_label_create(root, NULL);
    lv_label_set_text(label, title);
    lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
    
    // 创建开关按钮
    data->switch_btn = lv_btn_create(root, NULL);
    lv_obj_set_size(data->switch_btn, 120, 40);
    data->btn_label = lv_label_create(data->switch_btn, NULL);
    lv_label_set_text(data->btn_label, model->is_on ? "关闭" : "打开");
    lv_obj_align(data->switch_btn, NULL, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_user_data(data->switch_btn, page);
    lv_obj_set_event_cb(data->switch_btn, on_switch_click);
    
    // 创建亮度滑块
    data->slider = lv_slider_create(root, NULL);
    lv_obj_set_width(data->slider, 200);
    lv_obj_align(data->slider, NULL, LV_ALIGN_CENTER, 0, 30);
    lv_slider_set_range(data->slider, 0, 100);
    lv_slider_set_value(data->slider, model->brightness);
    lv_obj_set_user_data(data->slider, page);
    lv_obj_set_event_cb(data->slider, on_slider_change);
    
    page_set_user_data(page, data);
    
    // 订阅事件
    event_bus_subscribe(page, EVENT_LIGHT_STATE_CHANGED);
    event_bus_subscribe(page, EVENT_DEVICE_REMOVED);
}

static void light_page_on_resume(Page* page) {
    LightPageData* data = (LightPageData*)page_get_user_data(page);
    if (!data || !page_is_model_valid(page)) return;
    
    // 刷新UI
    lv_label_set_text(data->btn_label, data->model->is_on ? "关闭" : "打开");
    lv_slider_set_value(data->slider, data->model->brightness);
}

static void light_page_on_event(Page* page, Event* event) {
    LightPageData* data = (LightPageData*)page_get_user_data(page);
    if (!data) return;
    
    switch (event->type) {
        case EVENT_LIGHT_STATE_CHANGED:
            if (event->data.light.device_id == data->model->id) {
                // 更新UI
                lv_label_set_text(data->btn_label, 
                                  event->data.light.is_on ? "关闭" : "打开");
                lv_slider_set_value(data->slider, event->data.light.brightness);
            }
            break;
            
        case EVENT_DEVICE_REMOVED:
            if (event->data.device.device_id == data->model->id) {
                // 设备被移除
                page_set_model_valid(page, 0);
                
                // 减少引用计数
                if (data->model) {
                    data->model->ref_count--;
                }
                
                // 清空模型指针
                data->model = NULL;
                
                // 显示设备移除提示
                lv_obj_t* root = page_get_root(page);
                lv_obj_clean(root);
                
                lv_obj_t* label = lv_label_create(root, NULL);
                lv_label_set_text(label, "设备已移除");
                lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, -20);
                
                lv_obj_t* back_btn = lv_btn_create(root, NULL);
                lv_obj_t* btn_label = lv_label_create(back_btn, NULL);
                lv_label_set_text(btn_label, "返回");
                lv_obj_align(back_btn, NULL, LV_ALIGN_CENTER, 0, 20);
                lv_obj_set_event_cb(back_btn, on_back_click);
            }
            break;
            
        default:
            break;
    }
}

static void light_page_on_destroy(Page* page) {
    LightPageData* data = (LightPageData*)page_get_user_data(page);
    
    if (data && data->model) {
        data->model->ref_count--;
    }
    
    if (data) {
        free(data);
    }
    
    // 取消所有订阅
    event_bus_unsubscribe_all(page);
}

// 开关点击事件
static void on_switch_click(lv_obj_t* btn, lv_event_t event) {
    if (event != LV_EVENT_CLICKED) return;
    
    Page* page = (Page*)lv_obj_get_user_data(btn);
    if (!page || !page_is_model_valid(page)) {
        lv_label_set_text(lv_obj_get_child(btn, NULL), "无效");
        return;
    }
    
    LightPageData* data = (LightPageData*)page_get_user_data(page);
    if (!data || !data->model) return;
    
    int new_state = !data->model->is_on;
    
    // 发送命令到设备线程
    device_manager_send_command(g_dm, data->model->id, 0, new_state);
    
    // UI立即显示等待状态
    lv_label_set_text(data->btn_label, "发送中...");
}

// 滑块变化事件
static void on_slider_change(lv_obj_t* slider, lv_event_t event) {
    if (event != LV_EVENT_VALUE_CHANGED) return;
    
    Page* page = (Page*)lv_obj_get_user_data(slider);
    if (!page || !page_is_model_valid(page)) return;
    
    LightPageData* data = (LightPageData*)page_get_user_data(page);
    if (!data || !data->model) return;
    
    int brightness = lv_slider_get_value(slider);
    
    // 发送命令到设备线程
    device_manager_send_command(g_dm, data->model->id, 1, brightness);
}

// 返回按钮点击
static void on_back_click(lv_obj_t* btn, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
        page_manager_back();
    }
}

// 创建灯页面
static Page* light_page_create(void* params) {
    PageLifecycle cbs = {
        .on_create = light_page_on_create,
        .on_resume = light_page_on_resume,
        .on_event = light_page_on_event,
        .on_destroy = light_page_on_destroy
    };
    
    return page_create(&cbs, params);
}

// 主页面生命周期
static void main_page_on_create(Page* page, void* params) {
    (void)params;
    
    MainPageData* data = (MainPageData*)malloc(sizeof(MainPageData));
    memset(data, 0, sizeof(MainPageData));
    
    lv_obj_t* root = page_get_root(page);
    
    // 创建滑动容器
    data->swipe_container = swipe_container_create("tileview", root);
    lv_obj_set_size(data->swipe_container, 320, 400);
    lv_obj_align(data->swipe_container, NULL, LV_ALIGN_CENTER, 0, 0);
    
    // 为每个灯创建页面
    for (int i = 1; i <= 3; i++) {
        LightModel* model = device_manager_get_light(g_dm, i);
        if (model) {
            // 创建灯页面
            Page* light_page = light_page_create(model);
            
            // 添加到滑动容器
            lv_obj_t* tab = swipe_container_add_page(data->swipe_container, i-1);
            
            // 将灯页面的根容器移动到tab中
            lv_obj_t* light_root = page_get_root(light_page);
            lv_obj_set_parent(light_root, tab);
        }
    }
    
    page_set_user_data(page, data);
}

static void main_page_on_destroy(Page* page) {
    MainPageData* data = (MainPageData*)page_get_user_data(page);
    if (data) {
        free(data);
    }
}

static Page* main_page_create(void* params) {
    PageLifecycle cbs = {
        .on_create = main_page_on_create,
        .on_destroy = main_page_on_destroy
    };
    
    return page_create(&cbs, params);
}

// 主函数
int main(void) {
    // 初始化LVGL
    lv_init();
    
    // 初始化页面管理器
    page_manager_init();
    page_manager_set_cache_size(3);
    
    // 注册滑动容器实现
    swipe_container_register("tileview", &g_tileview_ops);
    
    // 注册页面
    page_manager_register("MainPage", main_page_create);
    page_manager_register("LightPage", light_page_create);
    
    // 初始化事件总线
    event_bus_init();
    
    // 创建设备管理器
    g_dm = device_manager_create();
    device_manager_start(g_dm);
    
    // 打开主页面
    page_manager_open("MainPage", NULL);
    
    // LVGL主循环
    while (1) {
        lv_task_handler();
        lv_thread_delay(5);
    }
    
    return 0;
}