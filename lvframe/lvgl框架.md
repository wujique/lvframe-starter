LVGL嵌入式UI框架完整设计方案
1. 概述
本方案为基于LVGL的嵌入式设备提供一套完整的UI管理框架，解决以下核心问题：

页面生命周期管理：统一管理页面的创建、显示、隐藏、销毁

导航栈管理：支持页面跳转、返回、历史记录

数据与视图分离：UI与业务数据解耦，支持多实例

线程安全通信：设备线程与UI线程之间安全、高效的消息传递

动态设备管理：支持设备的动态添加/移除，UI自动响应

整体架构分为三大核心模块：页面管理器、Model-View层、事件总线，并在此基础上扩展了设备移除处理机制。

2. 页面管理器
2.1 页面生命周期
页面具有明确的状态机，生命周期状态包括：

状态	说明	触发时机
创建	页面对象刚创建，UI元素初始化	onCreate
启动	页面即将可见，准备数据	onStart
恢复	页面完全可见，可交互	onResume
暂停	页面被部分覆盖（如新页面打开）	onPause
停止	页面完全不可见	onStop
销毁	页面即将释放资源	onDestroy
2.2 页面栈
采用栈结构维护页面历史

open操作：新页面压栈，当前页面暂停/停止

back操作：弹出栈顶页面，恢复上一页面

支持缓存策略：可配置缓存最近N个页面，避免频繁创建销毁

2.3 核心接口
c
// 页面操作
Page*        page_create(LifecycleCallbacks* cbs, void* init_params);
lv_obj_t*    page_get_root(Page* page);
void         page_set_user_data(Page* page, void* data);
void*        page_get_user_data(Page* page);

// 页面管理器初始化
void         page_manager_init(void);
void         page_manager_set_cache_size(int max_cache);

// 页面注册与导航
typedef Page* (*PageCreator)(void* params);
void         page_manager_register(const char* name, PageCreator creator);
void         page_manager_open(const char* name, void* params);
void         page_manager_back(void);
void         page_manager_back_to_home(void);
Page*        page_manager_get_current(void);

// 返回键处理
void         page_manager_handle_back_key(void);
2.4 滑动容器抽象
为了支持左右滑动页面（如多设备控制页），设计一个抽象的滑动容器接口，底层可切换不同LVGL组件实现（如lv_tileview、lv_win）。

c
// 滑动容器操作函数表
typedef struct {
    lv_obj_t* (*create)(lv_obj_t* parent);
    lv_obj_t* (*add_page)(lv_obj_t* container, int index);
    void      (*remove_page)(lv_obj_t* container, int index);
    void      (*switch_to)(lv_obj_t* container, int index);
    int       (*get_current)(lv_obj_t* container);
    int       (*get_count)(lv_obj_t* container);
} SwipeContainerOps;

// 注册与创建
void         swipe_container_register(const char* name, SwipeContainerOps* ops);
lv_obj_t*    swipe_container_create(const char* name, lv_obj_t* parent);
3. Model-View分离
3.1 设计原则
Model：独立的数据模型，包含设备状态、业务逻辑，不依赖UI。

View：即页面，只负责UI展示和用户交互，通过指针引用Model，但不拥有Model生命周期。

多实例支持：一套UI代码对应多个Model实例（如多个灯的控制界面）。

3.2 模型定义示例（灯模型）
c
typedef struct {
    int      id;
    char     name[16];
    bool     is_on;
    int      brightness;
    
    // 引用计数（被多少页面使用）
    int      ref_count;
    // 有效性标志
    bool     is_valid;
    // 模型失效时的回调（由框架调用）
    void     (*on_invalidated)(struct LightModel* model);
} LightModel;
3.3 模型管理
可选的模型管理器，负责创建、查询、销毁模型实例。

c
LightModel* light_model_create(int id, const char* name);
void        light_model_destroy(LightModel* model);
LightModel* light_model_find_by_id(int id);
3.4 页面与模型的绑定
页面创建时通过params传入模型指针。

页面持有模型指针，但不负责释放。

页面销毁时需解绑模型（减少引用计数）。

4. 事件总线（UI与设备通信）
4.1 设计目标
线程安全：设备线程（硬件轮询/控制）与LVGL UI线程分离。

轻量级：使用枚举事件类型，避免字符串解析。

订阅机制：页面只接收自己关心的事件，避免广播风暴。

非阻塞：设备线程发送事件后立即返回，不等待UI处理。

4.2 事件定义
c
// 预定义事件枚举
typedef enum {
    EVENT_LIGHT_STATE_CHANGED,   // 灯状态变化
    EVENT_DEVICE_ADDED,          // 新设备添加
    EVENT_DEVICE_REMOVED,        // 设备移除
    EVENT_NETWORK_DISCONNECTED,  // 网络断开
    // ... 其他事件
} EventType;

// 事件数据结构
typedef struct {
    EventType type;
    union {
        struct { int device_id; bool is_on; int brightness; } light;
        struct { int device_id; } device;
        // ... 其他数据类型
    } data;
} Event;
4.3 订阅机制
每个事件维护一个订阅者列表（页面指针数组）。

页面可订阅多个事件，多个页面可订阅同一事件。

c
// 订阅/取消订阅
void event_bus_subscribe(Page* page, EventType event);
void event_bus_unsubscribe(Page* page, EventType event);
void event_bus_unsubscribe_all(Page* page);
4.4 事件触发与分发
触发端（设备线程）：调用event_bus_trigger(Event* event)，将事件放入线程安全队列后立即返回。

分发端（LVGL线程）：通过LVGL定时任务定期调用event_bus_process()，从队列取出事件，查找订阅者，调用页面的on_event回调。

c
// 设备线程调用（非阻塞）
void event_bus_trigger(Event* event);

// LVGL定时任务中调用
void event_bus_process(void);

// 页面需实现的回调
void page_on_event(Page* page, Event* event);
4.5 线程安全设计
使用互斥锁保护事件队列和订阅列表。

事件队列采用无锁或简单锁实现，确保多线程安全。

5. 设备移除处理机制
当设备被物理移除或从系统中删除时，需要安全地清理相关UI和模型，避免野指针崩溃。

5.1 模型层增强
模型增加is_valid标志，标记是否有效。

模型增加引用计数ref_count，记录引用该模型的页面数量。

模型增加bound_pages列表（可选），方便反向查找。

5.2 移除流程
设备线程检测到设备移除 → 标记模型为is_valid = false。

触发EVENT_DEVICE_REMOVED事件，携带设备ID。

所有引用该模型的页面收到事件后：

标记页面内部状态model_valid = false。

解绑模型（减少引用计数，清空模型指针）。

切换UI为“设备已移除”状态（显示提示、禁用交互）。

主页面（滑动容器持有者）收到事件后：

从滑动容器中删除对应的tab页。

触发页面销毁，页面在onDestroy中取消订阅。

模型引用计数降为0时，真正释放模型内存。

5.3 页面交互保护
用户操作回调中首先检查model_valid标志，若无效则提示并忽略操作。

页面可提供“返回”按钮，让用户离开无效页面。

6. 完整流程示例（多灯控制应用）
6.1 初始化阶段
text
main()
 ├─ lvgl_init()
 ├─ page_manager_init(); page_manager_set_cache_size(3)
 ├─ swipe_container_register("tileview", tileview_ops)
 ├─ page_manager_register("MainPage", main_page_create)
 ├─ page_manager_register("LightPage", light_page_create)
 ├─ event_bus_init()
 ├─ device_manager_init()   // 创建3个灯模型
 ├─ device_thread_start()   // 启动设备线程
 └─ page_manager_open("MainPage", NULL)
6.2 主页面创建（带滑动容器）
text
main_page_on_create(page, params)
 ├─ 创建滑动容器 swipe = swipe_container_create("tileview", root)
 ├─ for i=1..3
 │    ├─ model = get_light_model(i)
 │    ├─ light_page = page_manager_create_page("LightPage", model)
 │    ├─ tab = swipe_container_add_page(swipe, i-1)
 │    └─ lvgl_set_parent(page_get_root(light_page), tab)
 └─ page_set_user_data(page, swipe)
6.3 灯页面创建与绑定
text
light_page_create(params)  // params = LightModel*
 ├─ page = page_create(lifecycle_callbacks, params)
 ├─ page_set_user_data(page, model)
 ├─ model->ref_count++
 ├─ 订阅事件：EVENT_LIGHT_STATE_CHANGED, EVENT_DEVICE_REMOVED
 └─ return page
6.4 用户点击开关
text
on_switch_click(btn)
 ├─ page = lvgl_get_user_data(btn)
 ├─ if (!page.model_valid) { show_toast("设备不存在"); return; }
 ├─ model = page_get_user_data(page)
 ├─ cmd = { device_id: model.id, command: TOGGLE }
 ├─ device_send_command(cmd)  // 放入设备命令队列
 └─ show_button_loading(btn)
6.5 设备状态变化上报
text
设备线程轮询到灯1状态变化
 ├─ 更新模型 model->is_on = new_state
 ├─ 创建事件 ev = { EVENT_LIGHT_STATE_CHANGED, { light_id:1, is_on:... } }
 └─ event_bus_trigger(ev)
 
LVGL定时任务中
 ├─ event_bus_process()
 │   ├─ 从队列取出事件
 │   ├─ 查找订阅 EVENT_LIGHT_STATE_CHANGED 的页面
 │   └─ 对每个页面调用 page_on_event(page, ev)
6.6 设备移除处理
text
设备线程检测到灯2移除
 ├─ model2.is_valid = false
 ├─ ev = { EVENT_DEVICE_REMOVED, { device_id:2 } }
 └─ event_bus_trigger(ev)

灯2页面收到事件
 ├─ page.model_valid = false
 ├─ 减少 model2.ref_count
 ├─ page_set_user_data(page, NULL)
 ├─ 切换UI为"设备已移除"
 └─ 禁用所有控件

主页面收到事件
 ├─ 从滑动容器中删除索引为2的tab
 └─ 灯2页面被销毁，onDestroy中取消订阅

model2.ref_count == 0 → 释放 model2
7. 接口汇总
页面相关
c
// 页面结构体（不透明）
typedef struct Page Page;

// 生命周期回调组
typedef struct {
    void (*on_create)(Page* page, void* params);
    void (*on_start)(Page* page);
    void (*on_resume)(Page* page);
    void (*on_pause)(Page* page);
    void (*on_stop)(Page* page);
    void (*on_destroy)(Page* page);
    void (*on_event)(Page* page, Event* event);  // 事件处理
} PageLifecycle;

Page* page_create(PageLifecycle* cbs, void* params);
lv_obj_t* page_get_root(Page* page);
void page_set_user_data(Page* page, void* data);
void* page_get_user_data(Page* page);
页面管理器
c
void page_manager_init(void);
void page_manager_set_cache_size(int max);
void page_manager_register(const char* name, Page* (*creator)(void*));
void page_manager_open(const char* name, void* params);
void page_manager_back(void);
void page_manager_back_to_home(void);
Page* page_manager_get_current(void);
void page_manager_handle_back_key(void);
滑动容器
c
typedef struct {
    lv_obj_t* (*create)(lv_obj_t* parent);
    lv_obj_t* (*add)(lv_obj_t* container, int index);
    void      (*remove)(lv_obj_t* container, int index);
    void      (*switch_to)(lv_obj_t* container, int index);
    int       (*get_current)(lv_obj_t* container);
    int       (*get_count)(lv_obj_t* container);
} SwipeContainerOps;

void swipe_container_register(const char* name, SwipeContainerOps* ops);
lv_obj_t* swipe_container_create(const char* name, lv_obj_t* parent);
事件总线
c
typedef enum { /* 事件枚举 */ } EventType;
typedef struct Event Event;  // 具体定义略

void event_bus_init(void);
void event_bus_subscribe(Page* page, EventType event);
void event_bus_unsubscribe(Page* page, EventType event);
void event_bus_unsubscribe_all(Page* page);
void event_bus_trigger(Event* event);     // 可由设备线程调用
void event_bus_process(void);              // 由LVGL定时任务调用
设备模型（示例）
c
typedef struct LightModel LightModel;

LightModel* light_model_create(int id, const char* name);
void light_model_destroy(LightModel* model);
void light_model_set_power(LightModel* model, bool on);
bool light_model_get_power(LightModel* model);
// ... 其他业务方法
8. 总结
本方案通过页面管理器、Model-View分离、事件总线三大核心组件，构建了一个清晰、可扩展的嵌入式UI框架。特点：

生命周期管理：规范页面行为，避免资源泄漏。

数据与UI解耦：支持多实例，便于维护。

线程安全通信：设备与UI分离，避免阻塞。

动态设备处理：安全应对设备添加/移除，提升系统稳定性。

该框架已成功应用于多灯控制等实际场景，具备良好的可移植性和扩展性。

