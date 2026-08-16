/**
 * @file         slot.h
 * @brief        消息槽（lv_slot）接口定义：任意线程发送，唯一消费者线程按
 *               signal + object 分发，payload 由槽负责释放，四元组退订
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef LV_SLOT_H
#define LV_SLOT_H

#include "../lvgl/lvgl/lvgl.h"
#include "../lvgl/lvgl/src/osal/lv_os_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 槽名称最大长度（含终止符） */
#define LV_SLOT_NAME_MAX 24

/**
 * @brief        消息骨架：signal / object / payload 语义完全由应用定义
 */
typedef struct {
    int   signal;                 /**< 信号（signal/event）：应用枚举 */
    void* object;                 /**< 对象：不透明指针（仅 == 比较）；NULL = 广播；槽永不释放、不读写内容 */
    void* payload;                /**< 负载，槽不关心内容；动态申请由槽负责释放；NULL = 无负载 */
    void  (*payload_dtor)(void*); /**< payload 析构（payload != NULL 时于分发结束后调用）：
                                   *   NULL        → 槽用 lv_free 释放；
                                   *   自定义函数  → 释放嵌套指针等；
                                   *   空函数 no-op → 不需要释放（借用/静态 payload）。 */
    int   arg0, arg1;             /**< 便捷整型参数（box86: value / new_pos）*/
    double argf;                  /**< 便捷浮点参数（如空调温度，0.1 精度）*/
    char  field[24];              /**< 便捷字符串参数（box86: 属性名）*/
} lv_slot_msg_t;

/** 订阅回调：ctx 为订阅时传入的对象，msg 为按信号 + 对象过滤后的完整消息 */
typedef void (*lv_slot_handler_t)(void* ctx, const lv_slot_msg_t* msg);

/** 消息节点：发送时动态申请（lv_malloc），分发后连同 payload 一起释放 */
typedef struct lv_slot_node {
    lv_slot_msg_t        msg;
    struct lv_slot_node* next;
} lv_slot_node_t;

/** 订阅者节点：订阅唯一身份 = (signal, object, handler, ctx) 四元组，动态申请，无数量上限 */
typedef struct lv_slot_sub {
    int                 signal;  /**< 订阅的信号类型 */
    void*               object;  /**< 订阅的对象；NULL = 该信号全接受（不管发送方是否指定对象）*/
    lv_slot_handler_t   handler; /**< 订阅回调；退订时作为身份一部分（函数指针 == 比较）*/
    void*               ctx;     /**< 订阅者对象（Page* 或任意）*/
    struct lv_slot_sub* next;
} lv_slot_sub_t;

/**
 * @brief        消息槽：动态链表消息队列 + 订阅者链表 + 唯一消费者线程
 */
typedef struct {
    char             name[LV_SLOT_NAME_MAX]; /**< 槽名："g_ui_slot" / "g_dev_slot" */
    lv_slot_node_t*  head;                   /**< 队头（消费者出队）*/
    lv_slot_node_t*  tail;                   /**< 队尾（发送方追加）*/
    int              count;                  /**< 待处理消息数（诊断）*/
    lv_mutex_t       mutex;                  /**< 保护队列与订阅链表 */
    lv_thread_sync_t sem;                    /**< 入队 signal，唤醒阻塞消费者 */
    lv_slot_sub_t*   subs;                   /**< 订阅者链表 */
    int              sub_count;
    int              dropped;                /**< 分配失败丢弃计数（诊断）*/
} lv_slot_t;

/**
 * @brief        初始化槽，清零并创建互斥锁与信号量
 */
void lv_slot_init(lv_slot_t* slot, const char* name);

/**
 * @brief        反初始化槽，释放全部残留消息（含 payload）/订阅节点，销毁锁与信号量
 */
void lv_slot_deinit(lv_slot_t* slot);

/**
 * @brief        任意线程向槽发送消息；非阻塞；payload 所有权一次性移交
 *
 * 节点分配失败时立即释放 payload（dropped++）并返回 -1。
 *
 * @return       成功 0，失败 -1
 */
int  lv_slot_send(lv_slot_t* slot, const lv_slot_msg_t* msg);

/**
 * @brief        按信号 + 对象订阅（object=NULL 全接受）；重复订阅去重；无数量上限
 *
 * @return       成功 0，失败 -1
 */
int  lv_slot_subscribe(lv_slot_t* slot, int signal, void* object,
                       lv_slot_handler_t handler, void* ctx);

/**
 * @brief        移除该 ctx 的全部订阅（page_destroy 自动调用）
 *
 * @return       移除返回 0，无匹配返回 -1
 */
int  lv_slot_unsubscribe_all(lv_slot_t* slot, void* ctx);

/**
 * @brief        精确退订：signal / object / handler / ctx 四元组全等才删除（对齐 Qt disconnect）
 *
 * @return       移除返回 0，无匹配返回 -1
 */
int  lv_slot_unsubscribe_one(lv_slot_t* slot, int signal, void* object,
                             lv_slot_handler_t handler, void* ctx);

/**
 * @brief        消费者线程：循环出队 → 快照 → 按 signal + object 分发 → 释放 payload 与节点
 */
void lv_slot_process(lv_slot_t* slot);

/**
 * @brief        阻塞等待消息（基于 lv_thread_sync_t），供纯消费者线程使用
 *
 * @return       被 signal 唤醒后返回 0
 */
int  lv_slot_wait(lv_slot_t* slot);

#ifdef __cplusplus
}
#endif

#endif /* LV_SLOT_H */
