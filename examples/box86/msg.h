/**
 * @file         msg.h
 * @brief        box86 信号化消息枚举（应用定义，框架零限制）
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef BOX86_MSG_H
#define BOX86_MSG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief        box86 消息信号枚举
 */
typedef enum {
    /* → g_dev_slot（DEV 线程消费）*/
    MSG_UI_SET_PROP,     /* object=模型, field=属性名, arg0=新值 */
    MSG_UI_SET_SYSTEM,   /* object=SYSTEM 模型, field=字段, arg0=新值 */
    MSG_UI_DEL_ACK,      /* object=模型（UI 已销毁页面，可释放）*/

    /* → g_ui_slot（UI 线程消费）：设备信号 */
    MSG_DEV_LIGHT_ON,        /* object=灯模型：灯打开 */
    MSG_DEV_LIGHT_OFF,       /* object=灯模型：灯关闭 */
    MSG_DEV_CCT_TEMP,        /* object=色温灯模型：色温变化 */
    MSG_DEV_CURTAIN_POS,     /* object=窗帘模型：开合度变化 */
    MSG_DEV_ALL_LIGHTS_OFF,  /* object=NULL：全屋断电广播（信号级广播示例）*/

    /* → g_ui_slot：生命周期 */
    MSG_DEV_ADD_MODEL,   /* object=模型（创建页面并绑定）*/
    MSG_DEV_DEL_MODEL,   /* object=模型（销毁绑定页面）*/
    MSG_DEV_MOVE_MODEL,  /* object=模型, arg0=new_pos */
    MSG_DEV_REFRESH_SYS, /* object=SYSTEM 模型 */
} box86_msg_type_t;

#ifdef __cplusplus
}
#endif

#endif /* BOX86_MSG_H */
