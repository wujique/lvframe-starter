# LVGL 嵌入式 UI 框架

基于 LVGL 的嵌入式设备 UI 管理框架，提供页面生命周期、导航栈、Model-View 分离、线程安全事件通信和动态设备管理。

## 目录结构

```
.
├── lvframe/                         # LvFrame 框架源码
│   ├── page.h / page.c              # 页面基础结构与生命周期
│   ├── page_manager.h / page_manager.c  # 页面管理器（导航栈、缓存）
│   ├── event_bus.h / event_bus.c    # 事件总线（线程安全）
│   ├── swipe_container.h / swipe_container.c  # 滑动容器抽象层
│   ├── device_manager.h / device_manager.c    # 设备管理器与灯模型
│   └── impl/tileview_impl.c         # lv_tileview 滑动容器实现
├── examples/light_control_app.c     # 多灯控制完整示例
├── lvgl框架.md                       # 高层架构设计方案
└── 详细设计文档.md                    # 详细设计与 API 文档
```

## 核心模块

### 1. Page（页面）

页面状态机：`NONE → CREATED → STARTED → RESUMED ⇄ PAUSED → STOPPED → DESTROYED`

```c
// page.h 关键结构
struct Page {
    lv_obj_t*    root;         // 根 LVGL 对象
    PageState    state;        // 当前状态
    void*        user_data;    // 用户数据（通常指向 Model）
    PageLifecycle lifecycle;   // 生命周期回调
    int          model_valid;  // 模型有效性标志（设备移除保护）
};
```

关键 API：
```c
Page*     page_create(PageLifecycle* lifecycle, void* params);
void      page_destroy(Page* page);
lv_obj_t* page_get_root(Page* page);
void      page_set_user_data(Page* page, void* data);
void*     page_get_user_data(Page* page);
void      page_set_model_valid(Page* page, int valid);
int       page_is_model_valid(Page* page);
```

### 2. PageManager（页面管理器）

- 页面栈最大深度：`MAX_PAGE_STACK = 20`
- 页面缓存最大数量：`MAX_CACHE_SIZE = 10`（FIFO 策略）
- 页面名称最大长度：`MAX_PAGE_NAME = 32`

```c
void   page_manager_init(void);
void   page_manager_set_cache_size(int size);
void   page_manager_register(const char* name, PageCreator creator);
void   page_manager_open(const char* name, void* params);
void   page_manager_back(void);
void   page_manager_back_to_home(void);
Page*  page_manager_get_current(void);
void   page_manager_handle_back_key(void);
Page*  page_manager_find_page_by_model(void* model);
```

### 3. EventBus（事件总线）

线程安全的发布/订阅机制，设备线程发布，UI 线程消费。

- 每种事件最多 `MAX_SUBSCRIBERS_PER_EVENT = 10` 个订阅者
- 事件队列容量：`MAX_EVENT_QUEUE = 50`（环形缓冲区）
- 处理间隔：50ms（LVGL 定时任务）

```c
// 预定义事件类型
typedef enum {
    EVENT_LIGHT_STATE_CHANGED,
    EVENT_DEVICE_ADDED,
    EVENT_DEVICE_REMOVED,
    EVENT_NETWORK_DISCONNECTED,
    EVENT_BATTERY_LOW,
    EVENT_COUNT
} EventType;

void event_bus_init(void);
void event_bus_subscribe(Page* page, EventType event);
void event_bus_unsubscribe(Page* page, EventType event);
void event_bus_unsubscribe_all(Page* page);
void event_bus_trigger(Event* event);   // 设备线程调用，非阻塞
void event_bus_process(void);           // LVGL 定时任务调用
```

### 4. SwipeContainer（滑动容器抽象层）

通过函数表抽象底层 LVGL 组件，支持多种实现切换。

```c
typedef struct {
    lv_obj_t* (*create)(lv_obj_t* parent);
    lv_obj_t* (*add_page)(lv_obj_t* container, int index);
    void      (*remove_page)(lv_obj_t* container, int index);
    void      (*switch_to)(lv_obj_t* container, int index);
    int       (*get_current)(lv_obj_t* container);
    int       (*get_count)(lv_obj_t* container);
} SwipeContainerOps;

void      swipe_container_register(const char* name, SwipeContainerOps* ops);
lv_obj_t* swipe_container_create(const char* name, lv_obj_t* parent);
// ... add_page / remove_page / switch_to / get_current / get_count
```

当前实现：`impl/tileview_impl.c` → `g_tileview_ops`（基于 `lv_tileview`）

### 5. DeviceManager（设备管理器）

管理灯模型，运行独立设备线程（100ms 轮询）。

```c
typedef struct LightModel {
    int  id;            // 设备 ID
    char name[16];      // 设备名称
    int  is_on;         // 开关状态
    int  brightness;    // 亮度 0-100
    int  ref_count;     // 页面引用计数
    int  is_valid;      // 有效性标志（0 = 已移除）
} LightModel;

DeviceManager* device_manager_create(void);
void           device_manager_start(DeviceManager* dm);
void           device_manager_stop(DeviceManager* dm);
LightModel*    device_manager_get_light(DeviceManager* dm, int id);
void           device_manager_send_command(DeviceManager* dm, int device_id, int cmd, int value);
// cmd: 0=开关, 1=亮度; value: 新状态值
```

## 初始化流程

```c
lv_init();
page_manager_init();
page_manager_set_cache_size(3);
swipe_container_register("tileview", &g_tileview_ops);
page_manager_register("MainPage", main_page_create);
event_bus_init();
DeviceManager* dm = device_manager_create();
device_manager_start(dm);
page_manager_open("MainPage", NULL);

while (1) {
    lv_task_handler();
    lv_thread_delay(5);
}
```

## 线程模型

| 线程 | 职责 |
|------|------|
| UI 线程（LVGL 主循环） | 渲染、用户输入、`event_bus_process()` |
| 设备线程 | 硬件轮询、命令处理、`event_bus_trigger()` |

共享数据通过 `lv_mutex_t` 保护。

## 设备移除处理

```
设备线程: model->is_valid = 0 → event_bus_trigger(EVENT_DEVICE_REMOVED)
    ↓
灯页面: page_set_model_valid(0) → ref_count-- → 显示"设备已移除"
    ↓
主页面: swipe_container_remove_page() → 销毁对应页面
    ↓
ref_count == 0 → 安全释放模型内存
```

## 扩展指南

- **新事件类型**：在 `event_bus.h` 的 `EventType` 枚举和 `Event` 联合体中添加
- **新设备类型**：创建新 Model 结构，扩展 `DeviceManager`，添加对应事件
- **新滑动容器实现**：实现 `SwipeContainerOps` 函数表，调用 `swipe_container_register` 注册

## 编译时配置

| 宏 | 默认值 | 说明 |
|----|--------|------|
| `MAX_PAGE_STACK` | 20 | 页面栈最大深度 |
| `MAX_CACHE_SIZE` | 10 | 页面缓存最大数量 |
| `MAX_EVENT_TYPES` | 20 | 事件类型最大数量 |
| `MAX_SUBSCRIBERS_PER_EVENT` | 10 | 每事件最大订阅者数 |
| `MAX_EVENT_QUEUE` | 50 | 事件队列容量 |
| `MAX_LIGHTS` | 10 | 最大设备数量 |

## 参考文档

- `lvgl框架.md` — 高层架构设计与设计决策
- `详细设计文档.md` — 完整 API 文档与实现细节
- `examples/light_control_app.c` — 多灯控制完整示例
