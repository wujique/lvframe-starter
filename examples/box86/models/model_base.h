/**
 * @file         model_base.h
 * @brief        box86 应用层通用模型头：所有模型结构体以 model_base_t 为首成员
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef BOX86_MODEL_BASE_H
#define BOX86_MODEL_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

/** 单个仓库支持的最大设备模型数量 */
#define MODEL_STORE_MAX 32

/** 模型名称最大字节数（含终止符） */
#define MODEL_NAME_MAX 32

/**
 * @brief        通用模型头（必须是所有模型结构体的第一个成员）
 *
 * 通过将模型结构体首地址强转为 model_base_t*，可统一访问 id/type/name/valid。
 */
typedef struct {
    int   id;          /**< 唯一 ID：设备从 1 递增；SYSTEM = 0 */
    int   type;        /**< 模型类型（box86_model_type_t）*/
    char  name[MODEL_NAME_MAX]; /**< 名称 */
    int   ref_count;   /**< 被 UI 绑定数（诊断）*/
    int   valid;       /**< 1 有效 / 0 已标记删除 */
} model_base_t;

/**
 * @brief        box86 模型类型枚举
 */
typedef enum {
    MODEL_TYPE_LIGHT = 0, /**< 普通灯 */
    MODEL_TYPE_CCT,       /**< 色温灯 */
    MODEL_TYPE_CURTAIN,   /**< 电动窗帘 */
    MODEL_TYPE_SYSTEM,    /**< 虚拟模型：亮度/音量/网络/屏保配置 */
    MODEL_TYPE_COUNT
} box86_model_type_t;

#ifdef __cplusplus
}
#endif

#endif /* BOX86_MODEL_BASE_H */
