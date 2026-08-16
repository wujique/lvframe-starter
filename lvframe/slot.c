/**
 * @file         slot.c
 * @brief        消息槽实现：动态链表队列 + 订阅者链表，按 signal + object 分发
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#include "slot.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief        释放 payload（按 payload_dtor 策略，缺省 lv_free）
 */
static void release_payload(lv_slot_msg_t* msg)
{
    if (msg->payload) {
        if (msg->payload_dtor) msg->payload_dtor(msg->payload);
        else lv_free(msg->payload);
        msg->payload = NULL;
    }
}

void lv_slot_init(lv_slot_t* slot, const char* name)
{
    if (!slot) return;
    memset(slot, 0, sizeof(*slot));
    strncpy(slot->name, name ? name : "", LV_SLOT_NAME_MAX - 1);
    slot->name[LV_SLOT_NAME_MAX - 1] = '\0';
    lv_mutex_init(&slot->mutex);
    lv_thread_sync_init(&slot->sem);
    printf("[Slot] init '%s'\n", slot->name);
}

void lv_slot_deinit(lv_slot_t* slot)
{
    if (!slot) return;
    printf("[Slot] deinit '%s'\n", slot->name);

    /* 释放残留消息（含 payload） */
    lv_slot_node_t* n = slot->head;
    while (n) {
        lv_slot_node_t* next = n->next;
        release_payload(&n->msg);
        lv_free(n);
        n = next;
    }
    slot->head = slot->tail = NULL;
    slot->count = 0;

    /* 释放订阅节点 */
    lv_slot_sub_t* s = slot->subs;
    while (s) {
        lv_slot_sub_t* next = s->next;
        lv_free(s);
        s = next;
    }
    slot->subs = NULL;
    slot->sub_count = 0;

    lv_thread_sync_delete(&slot->sem);
    lv_mutex_delete(&slot->mutex);
}

int lv_slot_send(lv_slot_t* slot, const lv_slot_msg_t* msg)
{
    if (!slot || !msg) return -1;

    lv_slot_node_t* node = lv_malloc(sizeof(*node));
    if (!node) {
        /* 所有权已移交：分配失败也要释放 payload，避免泄漏 */
        lv_slot_msg_t m = *msg;
        release_payload(&m);
        lv_mutex_lock(&slot->mutex);
        slot->dropped++;
        lv_mutex_unlock(&slot->mutex);
        printf("[Slot] '%s' send failed (alloc), dropped=%d\n", slot->name, slot->dropped);
        return -1;
    }

    node->msg = *msg;
    node->next = NULL;

    lv_mutex_lock(&slot->mutex);
    if (slot->tail) slot->tail->next = node;
    else slot->head = node;
    slot->tail = node;
    slot->count++;
    lv_mutex_unlock(&slot->mutex);

    lv_thread_sync_signal(&slot->sem);
    return 0;
}

int lv_slot_subscribe(lv_slot_t* slot, int signal, void* object,
                      lv_slot_handler_t handler, void* ctx)
{
    if (!slot || !handler || !ctx) return -1;

    lv_mutex_lock(&slot->mutex);

    /* 四元组去重 */
    for (lv_slot_sub_t* s = slot->subs; s; s = s->next) {
        if (s->signal == signal && s->object == object &&
            s->handler == handler && s->ctx == ctx) {
            lv_mutex_unlock(&slot->mutex);
            return 0;
        }
    }

    lv_slot_sub_t* sub = lv_malloc(sizeof(*sub));
    if (!sub) {
        lv_mutex_unlock(&slot->mutex);
        return -1;
    }
    sub->signal = signal;
    sub->object = object;
    sub->handler = handler;
    sub->ctx = ctx;
    sub->next = slot->subs;
    slot->subs = sub;
    slot->sub_count++;

    lv_mutex_unlock(&slot->mutex);
    return 0;
}

int lv_slot_unsubscribe_all(lv_slot_t* slot, void* ctx)
{
    if (!slot) return -1;

    lv_mutex_lock(&slot->mutex);
    lv_slot_sub_t** pp = &slot->subs;
    int removed = 0;
    while (*pp) {
        if ((*pp)->ctx == ctx) {
            lv_slot_sub_t* d = *pp;
            *pp = d->next;
            lv_free(d);
            slot->sub_count--;
            removed++;
        } else {
            pp = &(*pp)->next;
        }
    }
    lv_mutex_unlock(&slot->mutex);
    return removed ? 0 : -1;
}

int lv_slot_unsubscribe_one(lv_slot_t* slot, int signal, void* object,
                            lv_slot_handler_t handler, void* ctx)
{
    if (!slot) return -1;

    lv_mutex_lock(&slot->mutex);
    lv_slot_sub_t** pp = &slot->subs;
    int removed = 0;
    while (*pp) {
        lv_slot_sub_t* s = *pp;
        if (s->signal == signal && s->object == object &&
            s->handler == handler && s->ctx == ctx) {
            *pp = s->next;
            lv_free(s);
            slot->sub_count--;
            removed++;
            break; /* 四元组唯一，最多一个 */
        }
        pp = &(*pp)->next;
    }
    lv_mutex_unlock(&slot->mutex);
    return removed ? 0 : -1;
}

void lv_slot_process(lv_slot_t* slot)
{
    if (!slot) return;

    while (1) {
        /* 出队一条消息 */
        lv_slot_node_t* node = NULL;
        lv_mutex_lock(&slot->mutex);
        if (slot->count > 0) {
            node = slot->head;
            slot->head = node->next;
            if (!slot->head) slot->tail = NULL;
            slot->count--;
        }
        lv_mutex_unlock(&slot->mutex);
        if (!node) break;

        /* 锁内收集订阅者快照，再解锁分发（回调内可安全订阅/退订/再发送） */
        int n = 0;
        lv_slot_sub_t** snap = NULL;
        lv_mutex_lock(&slot->mutex);
        n = slot->sub_count;
        if (n > 0) {
            snap = lv_malloc(sizeof(void*) * (size_t)n);
            if (snap) {
                int i = 0;
                for (lv_slot_sub_t* s = slot->subs; s; s = s->next) snap[i++] = s;
            }
        }
        lv_mutex_unlock(&slot->mutex);

        if (snap) {
            for (int i = 0; i < n; i++) {
                lv_slot_sub_t* s = snap[i];
                /* signal 相等 且 object 匹配（NULL 双向通配） */
                if (s->signal == node->msg.signal &&
                    (s->object == NULL || node->msg.object == NULL ||
                     s->object == node->msg.object)) {
                    s->handler(s->ctx, &node->msg);
                }
            }
            lv_free(snap);
        }

        release_payload(&node->msg);
        lv_free(node);
    }
}

int lv_slot_wait(lv_slot_t* slot)
{
    if (!slot) return -1;
    lv_thread_sync_wait(&slot->sem);
    return 0;
}
